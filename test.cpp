#include "llvm/ADT/Triple.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/CodeGen/LinkAllAsmWriterComponents.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Target/TargetLibraryInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetSubtargetInfo.h"

#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Bitcode/BitcodeWriterPass.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassNameParser.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/InitializePasses.h"
#include "llvm/LinkAllIR.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/PassManager.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Target/TargetLibraryInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace libHLC {

static llvm::LLVMContext *TheContext = 0;

bool DisableInline = false;
bool UnitAtATime = false;
bool DisableLoopVectorization = false;
bool DisableSLPVectorization = false;
bool StripDebug = false;
bool DisableOptimizations = false;
bool DisableSimplifyLibCalls = false;

bool OptLevelO1 = false;
bool OptLevelO2 = false;
bool OptLevelO3 = false;
bool OptLevelOz = false;
bool OptLevelOs = false;
bool NoVerify = false;

CodeGenOpt::Level GetCodeGenOptLevel() {
  if (OptLevelO1)
    return CodeGenOpt::Less;
  if (OptLevelO2)
    return CodeGenOpt::Default;
  if (OptLevelO3)
    return CodeGenOpt::Aggressive;
  return CodeGenOpt::None;
}

//// Borrowed from LLVM opt.cpp
/// This routine adds optimization passes based on selected optimization level,
/// OptLevel.
///
/// OptLevel - Optimization Level
static void AddOptimizationPasses(PassManagerBase &MPM,FunctionPassManager &FPM,
                                  unsigned OptLevel, unsigned SizeLevel) {
  FPM.add(createVerifierPass());          // Verify that input is correct
  MPM.add(createDebugInfoVerifierPass()); // Verify that debug info is correct

  PassManagerBuilder Builder;
  Builder.OptLevel = OptLevel;
  Builder.SizeLevel = SizeLevel;

  if (DisableInline) {
    // No inlining pass
  } else if (OptLevel > 1) {
    Builder.Inliner = createFunctionInliningPass(OptLevel, SizeLevel);
  } else {
    Builder.Inliner = createAlwaysInlinerPass();
  }
  Builder.DisableUnitAtATime = !UnitAtATime;
  // Builder.DisableUnrollLoops = (DisableLoopUnrolling.getNumOccurrences() > 0) ?
  //                              DisableLoopUnrolling : OptLevel == 0;
  Builder.DisableUnrollLoops = OptLevel == 0;

  // This is final, unless there is a #pragma vectorize enable
  if (DisableLoopVectorization)
    Builder.LoopVectorize = false;
  // If option wasn't forced via cmd line (-vectorize-loops, -loop-vectorize)
  else if (!Builder.LoopVectorize)
    Builder.LoopVectorize = OptLevel > 1 && SizeLevel < 2;

  // When #pragma vectorize is on for SLP, do the same as above
  Builder.SLPVectorize =
      DisableSLPVectorization ? false : OptLevel > 1 && SizeLevel < 2;

  Builder.populateFunctionPassManager(FPM);
  Builder.populateModulePassManager(MPM);
}


// Returns the TargetMachine instance or zero if no triple is provided.
static TargetMachine* GetTargetMachine(Triple TheTriple) {
  std::string Error;
  const Target *TheTarget = TargetRegistry::lookupTarget(MArch, TheTriple,
                                                         Error);
  // Some modules don't specify a triple, and this is okay.
  if (!TheTarget) {
    return nullptr;
  }

  // Package up features to be passed to target/subtarget
  std::string FeaturesStr;
  if (MAttrs.size()) {
    SubtargetFeatures Features;
    for (unsigned i = 0; i != MAttrs.size(); ++i)
      Features.AddFeature(MAttrs[i]);
    FeaturesStr = Features.getString();
  }

  return TheTarget->createTargetMachine(TheTriple.getTriple(),
                                        MCPU, FeaturesStr,
                                        InitTargetOptionsFromCodeGenFlags(),
                                        RelocModel, CMModel,
                                        GetCodeGenOptLevel());
}


void Initialize() {
  using namespace llvm;

  if ( TheContext == 0 ) {
    // Already initialized
    return;
  }

  sys::PrintStackTraceOnErrorSignal();
  static const char* argv[1] = {"libHLC"};
  PrettyStackTraceProgram _(1, argv);

  // Enable debug stream buffering.
  EnableDebugBuffering = true;

  LLVMContext &Context = getGlobalContext();
  TheContext = &Context;

  // Initialize targets
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmPrinters();
  InitializeAllAsmParsers();

  // Initialize passes
  PassRegistry &Registry = *PassRegistry::getPassRegistry();
  initializeCore(Registry);
  initializeScalarOpts(Registry);
  initializeObjCARCOpts(Registry);
  initializeVectorization(Registry);
  initializeIPO(Registry);
  initializeAnalysis(Registry);
  initializeIPA(Registry);
  initializeTransformUtils(Registry);
  initializeInstCombine(Registry);
  initializeInstrumentation(Registry);
  initializeTarget(Registry);
  // For codegen passes, only passes that do IR to IR transformation are
  // supported.
  initializeCodeGenPreparePass(Registry);
  initializeAtomicExpandPass(Registry);
  initializeRewriteSymbolsPass(Registry);

  initializeCodeGen(Registry);
  initializeLoopStrengthReducePass(Registry);
  initializeLowerIntrinsicsPass(Registry);
  initializeUnreachableBlockElimPass(Registry);
}

void Finalize() {
  using namespace llvm;

  llvm_shutdown();
}

void Optimize(llvm::Module *M) {

    // Create a PassManager to hold and optimize the collection of passes we are
    // about to build.
    //
    PassManager Passes;

    // Add an appropriate TargetLibraryInfo pass for the module's triple.
    TargetLibraryInfo *TLI = new TargetLibraryInfo(Triple(M->getTargetTriple()));

    // The -disable-simplify-libcalls flag actually disables all builtin optzns.
    if (DisableSimplifyLibCalls)
      TLI->disableAllFunctions();
    Passes.add(TLI);

    // Add an appropriate DataLayout instance for this module.
    const DataLayout *DL = M->getDataLayout();
    if (DL)
      Passes.add(new DataLayoutPass());

    Triple ModuleTriple(M->getTargetTriple());
    TargetMachine *Machine = nullptr;
    if (ModuleTriple.getArch())
      Machine = GetTargetMachine(Triple(ModuleTriple));
    std::unique_ptr<TargetMachine> TM(Machine);

    // Add internal analysis passes from the target machine.
    if (TM.get())
      TM->addAnalysisPasses(Passes);

    std::unique_ptr<FunctionPassManager> FPasses;
    if (OptLevelO1 || OptLevelO2 || OptLevelOs || OptLevelOz || OptLevelO3) {
      FPasses.reset(new FunctionPassManager(M));
      if (DL)
        FPasses->add(new DataLayoutPass());
      if (TM.get())
        TM->addAnalysisPasses(*FPasses);

    }
    //
    // // If the -strip-debug command line option was specified, add it.
    // if (StripDebug)
    //   addPass(Passes, createStripSymbolsPass(true));

    if (OptLevelO1)
      AddOptimizationPasses(Passes, *FPasses, 1, 0);

    if (OptLevelO2)
      AddOptimizationPasses(Passes, *FPasses, 2, 0);

    if (OptLevelOs)
      AddOptimizationPasses(Passes, *FPasses, 2, 1);

    if (OptLevelOz)
      AddOptimizationPasses(Passes, *FPasses, 2, 2);

    if (OptLevelO3)
      AddOptimizationPasses(Passes, *FPasses, 3, 0);

    if (OptLevelO1 || OptLevelO2 || OptLevelOs || OptLevelOz || OptLevelO3) {
      FPasses->doInitialization();
      for (Module::iterator F = M->begin(), E = M->end(); F != E; ++F)
        FPasses->run(*F);
      FPasses->doFinalization();
    }

    // Check that the module is well formed on completion of optimization
    if (!NoVerify) {
      Passes.add(createVerifierPass());
      Passes.add(createDebugInfoVerifierPass());
    }

    // Now that we have all of the passes ready, run them.
    Passes.run(*M);
}

} // end libHLC namespace

extern "C" {

using namespace libHLC;

typedef struct OpaqueModule* llvm_module_ptr;

void libHLC_Initialize() {
  Initialize();
}

void libHLC_Finalize() {
  Initialize();
}

void libHLC_Optimize(llvm_module_ptr module) {
  llvm::Module *M = reinterpret_cast<llvm::Module*>(module);
  Optimize(M);
}

} // end extern "C"
