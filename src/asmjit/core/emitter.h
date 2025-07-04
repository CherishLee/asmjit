// This file is part of AsmJit project <https://asmjit.com>
//
// See <asmjit/core.h> or LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifndef ASMJIT_CORE_EMITTER_H_INCLUDED
#define ASMJIT_CORE_EMITTER_H_INCLUDED

#include "../core/archtraits.h"
#include "../core/codeholder.h"
#include "../core/formatter.h"
#include "../core/inst.h"
#include "../core/operand.h"
#include "../core/type.h"

ASMJIT_BEGIN_NAMESPACE

//! \addtogroup asmjit_core
//! \{

class ConstPool;
class FuncFrame;
class FuncArgsAssignment;

//! Align mode, used by \ref BaseEmitter::align().
enum class AlignMode : uint8_t {
  //! Align executable code.
  kCode = 0,
  //! Align non-executable code.
  kData = 1,
  //! Align by a sequence of zeros.
  kZero = 2,

  //! Maximum value of `AlignMode`.
  kMaxValue = kZero
};

//! Emitter type used by \ref BaseEmitter.
enum class EmitterType : uint8_t {
  //! Unknown or uninitialized.
  kNone = 0,
  //! Emitter inherits from \ref BaseAssembler.
  kAssembler = 1,
  //! Emitter inherits from \ref BaseBuilder.
  kBuilder = 2,
  //! Emitter inherits from \ref BaseCompiler.
  kCompiler = 3,

  //! Maximum value of `EmitterType`.
  kMaxValue = kCompiler
};

//! Emitter flags, used by \ref BaseEmitter.
enum class EmitterFlags : uint8_t {
  //! No flags.
  kNone = 0u,
  //! Emitter is attached to CodeHolder.
  kAttached = 0x01u,
  //! The emitter must emit comments.
  kLogComments = 0x08u,
  //! The emitter has its own \ref Logger (not propagated from \ref CodeHolder).
  kOwnLogger = 0x10u,
  //! The emitter has its own \ref ErrorHandler (not propagated from \ref CodeHolder).
  kOwnErrorHandler = 0x20u,
  //! The emitter was finalized.
  kFinalized = 0x40u,
  //! The emitter was destroyed.
  //!
  //! This flag is used for a very short time when an emitter is being destroyed by
  //! CodeHolder.
  kDestroyed = 0x80u
};
ASMJIT_DEFINE_ENUM_FLAGS(EmitterFlags)

//! Encoding options.
enum class EncodingOptions : uint32_t {
  //! No encoding options.
  kNone = 0,

  //! Emit instructions that are optimized for size, if possible.
  //!
  //! Default: false.
  //!
  //! X86 Specific
  //! ------------
  //!
  //! When this option is set it the assembler will try to fix instructions if possible into operation equivalent
  //! instructions that take less bytes by taking advantage of implicit zero extension. For example instruction
  //! like `mov r64, imm` and `and r64, imm` can be translated to `mov r32, imm` and `and r32, imm` when the
  //! immediate constant is lesser than `2^31`.
  kOptimizeForSize = 0x00000001u,

  //! Emit optimized code-alignment sequences.
  //!
  //! Default: false.
  //!
  //! X86 Specific
  //! ------------
  //!
  //! Default align sequence used by X86 architecture is one-byte (0x90) opcode that is often shown by disassemblers
  //! as NOP. However there are more optimized align sequences for 2-11 bytes that may execute faster on certain CPUs.
  //! If this feature is enabled AsmJit will generate specialized sequences for alignment between 2 to 11 bytes.
  kOptimizedAlign = 0x00000002u,

  //! Emit jump-prediction hints.
  //!
  //! Default: false.
  //!
  //! X86 Specific
  //! ------------
  //!
  //! Jump prediction is usually based on the direction of the jump. If the jump is backward it is usually predicted as
  //! taken; and if the jump is forward it is usually predicted as not-taken. The reason is that loops generally use
  //! backward jumps and conditions usually use forward jumps. However this behavior can be overridden by using
  //! instruction prefixes. If this option is enabled these hints will be emitted.
  //!
  //! This feature is disabled by default, because the only processor that used to take into consideration prediction
  //! hints was P4. Newer processors implement heuristics for branch prediction and ignore static hints. This means
  //! that this feature can be only used for annotation purposes.
  kPredictedJumps = 0x00000010u
};
ASMJIT_DEFINE_ENUM_FLAGS(EncodingOptions)

//! Diagnostic options are used to tell emitters and their passes to perform diagnostics when emitting or processing
//! user code. These options control validation and extra diagnostics that can be performed by higher level emitters.
//!
//! Instruction Validation
//! ----------------------
//!
//! \ref BaseAssembler implementation perform by default only basic checks that are necessary to identify all
//! variations of an instruction so the correct encoding can be selected. This is fine for production-ready code
//! as the assembler doesn't have to perform checks that would slow it down. However, sometimes these checks are
//! beneficial especially when the project that uses AsmJit is in a development phase, in which mistakes happen
//! often. To make the experience of using AsmJit seamless it offers validation features that can be controlled
//! by \ref DiagnosticOptions.
//!
//! Compiler Diagnostics
//! --------------------
//!
//! Diagnostic options work with \ref BaseCompiler passes (precisely with its register allocation pass). These options
//! can be used to enable logging of all operations that the Compiler does.
enum class DiagnosticOptions : uint32_t {
  //! No validation options.
  kNone = 0,

  //! Perform strict validation in \ref BaseAssembler::emit() implementations.
  //!
  //! This flag ensures that each instruction is checked before it's encoded into a binary representation. This flag
  //! is only relevant for \ref BaseAssembler implementations, but can be set in any other emitter type, in that case
  //! if that emitter needs to create an assembler on its own, for the purpose of \ref BaseEmitter::finalize() it
  //! would propagate this flag to such assembler so all instructions passed to it are explicitly validated.
  //!
  //! Default: false.
  kValidateAssembler = 0x00000001u,

  //! Perform strict validation in \ref BaseBuilder::emit() and \ref BaseCompiler::emit() implementations.
  //!
  //! This flag ensures that each instruction is checked before an \ref InstNode representing the instruction is
  //! created by \ref BaseBuilder or \ref BaseCompiler. This option could be more useful than \ref kValidateAssembler
  //! in cases in which there is an invalid instruction passed to an assembler, which was invalid much earlier, most
  //! likely when such instruction was passed to Builder/Compiler.
  //!
  //! This is a separate option that was introduced, because it's possible to manipulate the instruction stream
  //! emitted by \ref BaseBuilder and \ref BaseCompiler - this means that it's allowed to emit invalid instructions
  //! (for example with missing operands) that will be fixed later before finalizing it.
  //!
  //! Default: false.
  kValidateIntermediate = 0x00000002u,

  //! Annotate all nodes processed by register allocator (Compiler/RA).
  //!
  //! \note Annotations don't need debug options, however, some debug options like `kRADebugLiveness` may influence
  //! their output (for example the mentioned option would add liveness information to per-instruction annotation).
  kRAAnnotate = 0x00000080u,

  //! Debug CFG generation and other related algorithms / operations (Compiler/RA).
  kRADebugCFG = 0x00000100u,

  //! Debug liveness analysis (Compiler/RA).
  kRADebugLiveness = 0x00000200u,

  //! Debug register allocation assignment (Compiler/RA).
  kRADebugAssignment = 0x00000400u,

  //! Debug the removal of code part of unreachable blocks.
  kRADebugUnreachable = 0x00000800u,

  //! Enable all debug options (Compiler/RA).
  kRADebugAll = 0x0000FF00u
};
ASMJIT_DEFINE_ENUM_FLAGS(DiagnosticOptions)

//! Provides a base foundation to emitting code - specialized by \ref BaseAssembler and \ref BaseBuilder.
class ASMJIT_VIRTAPI BaseEmitter {
public:
  ASMJIT_BASE_CLASS(BaseEmitter)
  ASMJIT_NONCOPYABLE(BaseEmitter)

  //! \name Types
  //! \{

  //! Emitter state that can be used to specify options and inline comment of a next node or instruction.
  struct State {
    InstOptions options;
    RegOnly extraReg;
    const char* comment;
  };

  //! Functions used by backend-specific emitter implementation.
  //!
  //! These are typically shared between Assembler/Builder/Compiler of a single backend.
  struct Funcs {
    using EmitProlog = Error (ASMJIT_CDECL*)(BaseEmitter* emitter, const FuncFrame& frame);
    using EmitEpilog = Error (ASMJIT_CDECL*)(BaseEmitter* emitter, const FuncFrame& frame);
    using EmitArgsAssignment = Error (ASMJIT_CDECL*)(BaseEmitter* emitter, const FuncFrame& frame, const FuncArgsAssignment& args);

    using FormatInstruction = Error (ASMJIT_CDECL*)(
      String& sb,
      FormatFlags formatFlags,
      const BaseEmitter* emitter,
      Arch arch,
      const BaseInst& inst, const Operand_* operands, size_t opCount) noexcept;

    using ValidateFunc = Error (ASMJIT_CDECL*)(const BaseInst& inst, const Operand_* operands, size_t opCount, ValidationFlags validationFlags) noexcept;

    //! Emit prolog implementation.
    EmitProlog emitProlog;
    //! Emit epilog implementation.
    EmitEpilog emitEpilog;
    //! Emit arguments assignment implementation.
    EmitArgsAssignment emitArgsAssignment;
    //! Instruction formatter implementation.
    FormatInstruction formatInstruction;
    //! Instruction validation implementation.
    ValidateFunc validate;

    //! Resets all functions to nullptr.
    ASMJIT_INLINE_NODEBUG void reset() noexcept { *this = Funcs{}; }
  };

  //! \}

  //! \name Members
  //! \{

  //! See \ref EmitterType.
  EmitterType _emitterType = EmitterType::kNone;

  //! See \ref EmitterFlags.
  EmitterFlags _emitterFlags = EmitterFlags::kNone;

  //! Instruction alignment.
  uint8_t _instructionAlignment = 0u;

  //! Validation flags in case validation is used.
  //!
  //! \note Validation flags are specific to the emitter and they are setup at construction time and then never
  //! changed.
  ValidationFlags _validationFlags = ValidationFlags::kNone;

  //! Validation options.
  DiagnosticOptions _diagnosticOptions = DiagnosticOptions::kNone;

  //! Encoding options.
  EncodingOptions _encodingOptions = EncodingOptions::kNone;

  //! Forced instruction options, combined with \ref _instOptions by \ref emit().
  InstOptions _forcedInstOptions = InstOptions::kReserved;

  //! All supported architectures in a bit-mask, where LSB is the bit with a zero index.
  uint64_t _archMask = 0;

  //! CodeHolder the emitter is attached to.
  CodeHolder* _code = nullptr;

  //! Attached \ref Logger.
  Logger* _logger = nullptr;

  //! Attached \ref ErrorHandler.
  ErrorHandler* _errorHandler = nullptr;

  //! Describes the target environment, matches \ref CodeHolder::environment().
  Environment _environment {};

  //! Native GP register signature (either a 32-bit or 64-bit GP register signature).
  OperandSignature _gpSignature {};
  //! Internal private data used freely by any emitter.
  uint32_t _privateData = 0;

  //! Next instruction options (affects the next instruction).
  InstOptions _instOptions = InstOptions::kNone;
  //! Extra register (op-mask {k} on AVX-512) (affects the next instruction).
  RegOnly _extraReg {};
  //! Inline comment of the next instruction (affects the next instruction).
  const char* _inlineComment = nullptr;

  //! Pointer to functions used by backend-specific emitter implementation.
  Funcs _funcs {};

  //! Emitter attached before this emitter in \ref CodeHolder, otherwise nullptr if there is no emitter before.
  BaseEmitter* _attachedPrev = nullptr;
  //! Emitter attached after this emitter in \ref CodeHolder, otherwise nullptr if there is no emitter after.
  BaseEmitter* _attachedNext = nullptr;

  //! \}

  //! \name Construction & Destruction
  //! \{

  ASMJIT_API explicit BaseEmitter(EmitterType emitterType) noexcept;
  ASMJIT_API virtual ~BaseEmitter() noexcept;

  //! \}

  //! \name Cast
  //! \{

  template<typename T>
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG T* as() noexcept { return reinterpret_cast<T*>(this); }

  template<typename T>
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG const T* as() const noexcept { return reinterpret_cast<const T*>(this); }

  //! \}

  //! \name Emitter Type & Flags
  //! \{

  //! Returns the type of this emitter, see `EmitterType`.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG EmitterType emitterType() const noexcept { return _emitterType; }

  //! Returns emitter flags , see `Flags`.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG EmitterFlags emitterFlags() const noexcept { return _emitterFlags; }

  //! Tests whether the emitter inherits from `BaseAssembler`.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool isAssembler() const noexcept { return _emitterType == EmitterType::kAssembler; }

  //! Tests whether the emitter inherits from `BaseBuilder`.
  //!
  //! \note Both Builder and Compiler emitters would return `true`.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool isBuilder() const noexcept { return uint32_t(_emitterType) >= uint32_t(EmitterType::kBuilder); }

  //! Tests whether the emitter inherits from `BaseCompiler`.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool isCompiler() const noexcept { return _emitterType == EmitterType::kCompiler; }

  //! Tests whether the emitter has the given `flag` enabled.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool hasEmitterFlag(EmitterFlags flag) const noexcept { return Support::test(_emitterFlags, flag); }

  //! Tests whether the emitter is finalized.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool isFinalized() const noexcept { return hasEmitterFlag(EmitterFlags::kFinalized); }

  //! Tests whether the emitter is destroyed (only used during destruction).
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool isDestroyed() const noexcept { return hasEmitterFlag(EmitterFlags::kDestroyed); }

  //! \}

  //! \cond INTERNAL
  //! \name Internal Functions
  //! \{

  ASMJIT_INLINE_NODEBUG void _addEmitterFlags(EmitterFlags flags) noexcept { _emitterFlags |= flags; }
  ASMJIT_INLINE_NODEBUG void _clearEmitterFlags(EmitterFlags flags) noexcept { _emitterFlags &= _emitterFlags & ~flags; }

  //! \}
  //! \endcond

  //! \name Target Information
  //! \{

  //! Returns the CodeHolder this emitter is attached to.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG CodeHolder* code() const noexcept { return _code; }

  //! Returns the target environment.
  //!
  //! The returned \ref Environment reference matches \ref CodeHolder::environment().
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG const Environment& environment() const noexcept { return _environment; }

  //! Tests whether the target architecture is 32-bit.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool is32Bit() const noexcept { return environment().is32Bit(); }

  //! Tests whether the target architecture is 64-bit.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool is64Bit() const noexcept { return environment().is64Bit(); }

  //! Returns the target architecture type.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG Arch arch() const noexcept { return environment().arch(); }

  //! Returns the target architecture sub-type.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG SubArch subArch() const noexcept { return environment().subArch(); }

  //! Returns the target architecture's GP register size (4 or 8 bytes).
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG uint32_t registerSize() const noexcept { return environment().registerSize(); }

  //! Returns a signature of a native general purpose register (either 32-bit or 64-bit depending on the architecture).
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG OperandSignature gpSignature() const noexcept { return _gpSignature; }

  //! Returns instruction alignment.
  //!
  //! The following values are returned based on the target architecture:
  //!   - X86 and X86_64 - instruction alignment is 1
  //!   - AArch32 - instruction alignment is 4 in A32 mode and 2 in THUMB mode.
  //!   - AArch64 - instruction alignment is 4
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG uint32_t instructionAlignment() const noexcept { return _instructionAlignment; }

  //! \}

  //! \name Initialization & Finalization
  //! \{

  //! Tests whether the emitter is initialized (i.e. attached to \ref CodeHolder).
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool isInitialized() const noexcept { return _code != nullptr; }

  //! Finalizes this emitter.
  //!
  //! Materializes the content of the emitter by serializing it to the attached \ref CodeHolder through an architecture
  //! specific \ref BaseAssembler. This function won't do anything if the emitter inherits from \ref BaseAssembler as
  //! assemblers emit directly to a \ref CodeBuffer held by \ref CodeHolder. However, if this is an emitter that
  //! inherits from \ref BaseBuilder or \ref BaseCompiler then these emitters need the materialization phase as they
  //! store their content in a representation not visible to \ref CodeHolder.
  ASMJIT_API virtual Error finalize();

  //! \}

  //! \name Logging
  //! \{

  //! Tests whether the emitter has a logger.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool hasLogger() const noexcept { return _logger != nullptr; }

  //! Tests whether the emitter has its own logger.
  //!
  //! Own logger means that it overrides the possible logger that may be used by \ref CodeHolder this emitter is
  //! attached to.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool hasOwnLogger() const noexcept { return hasEmitterFlag(EmitterFlags::kOwnLogger); }

  //! Returns the logger this emitter uses.
  //!
  //! The returned logger is either the emitter's own logger or it's logger used by \ref CodeHolder this emitter
  //! is attached to.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG Logger* logger() const noexcept { return _logger; }

  //! Sets or resets the logger of the emitter.
  //!
  //! If the `logger` argument is non-null then the logger will be considered emitter's own logger, see \ref
  //! hasOwnLogger() for more details. If the given `logger` is null then the emitter will automatically use logger
  //! that is attached to the \ref CodeHolder this emitter is attached to.
  ASMJIT_API void setLogger(Logger* logger) noexcept;

  //! Resets the logger of this emitter.
  //!
  //! The emitter will bail to using a logger attached to \ref CodeHolder this emitter is attached to, or no logger
  //! at all if \ref CodeHolder doesn't have one.
  ASMJIT_INLINE_NODEBUG void resetLogger() noexcept { return setLogger(nullptr); }

  //! \}

  //! \name Error Handling
  //! \{

  //! Tests whether the emitter has an error handler attached.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool hasErrorHandler() const noexcept { return _errorHandler != nullptr; }

  //! Tests whether the emitter has its own error handler.
  //!
  //! Own error handler means that it overrides the possible error handler that may be used by \ref CodeHolder this
  //! emitter is attached to.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool hasOwnErrorHandler() const noexcept { return hasEmitterFlag(EmitterFlags::kOwnErrorHandler); }

  //! Returns the error handler this emitter uses.
  //!
  //! The returned error handler is either the emitter's own error handler or it's error handler used by
  //! \ref CodeHolder this emitter is attached to.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG ErrorHandler* errorHandler() const noexcept { return _errorHandler; }

  //! Sets or resets the error handler of the emitter.
  ASMJIT_API void setErrorHandler(ErrorHandler* errorHandler) noexcept;

  //! Resets the error handler.
  ASMJIT_INLINE_NODEBUG void resetErrorHandler() noexcept { setErrorHandler(nullptr); }

  //! \cond INTERNAL
  ASMJIT_API Error _reportError(Error err, const char* message = nullptr);
  //! \endcond

  //! Handles the given error in the following way:
  //!   1. If the emitter has \ref ErrorHandler attached, it calls its \ref ErrorHandler::handleError() member function
  //!      first, and then returns the error. The `handleError()` function may throw.
  //!   2. if the emitter doesn't have \ref ErrorHandler, the error is simply returned.
  ASMJIT_INLINE Error reportError(Error err, const char* message = nullptr) {
    Error e = _reportError(err, message);

    // Static analysis is not working properly without these assumptions.
    ASMJIT_ASSUME(e == err);
    ASMJIT_ASSUME(e != kErrorOk);

    return e;
  }

  //! \}

  //! \name Encoding Options
  //! \{

  //! Returns encoding options.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG EncodingOptions encodingOptions() const noexcept { return _encodingOptions; }

  //! Tests whether the encoding `option` is set.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool hasEncodingOption(EncodingOptions option) const noexcept { return Support::test(_encodingOptions, option); }

  //! Enables the given encoding `options`.
  ASMJIT_INLINE_NODEBUG void addEncodingOptions(EncodingOptions options) noexcept { _encodingOptions |= options; }
  //! Disables the given encoding `options`.
  ASMJIT_INLINE_NODEBUG void clearEncodingOptions(EncodingOptions options) noexcept { _encodingOptions &= ~options; }

  //! \}

  //! \name Diagnostic Options
  //! \{

  //! Returns the emitter's diagnostic options.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG DiagnosticOptions diagnosticOptions() const noexcept { return _diagnosticOptions; }

  //! Tests whether the given `option` is present in the emitter's diagnostic options.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool hasDiagnosticOption(DiagnosticOptions option) const noexcept { return Support::test(_diagnosticOptions, option); }

  //! Activates the given diagnostic `options`.
  //!
  //! This function is used to activate explicit validation options that will be then used by all emitter
  //! implementations. There are in general two possibilities:
  //!
  //!   - Architecture specific assembler is used. In this case a \ref DiagnosticOptions::kValidateAssembler can be
  //!     used to turn on explicit validation that will be used before an instruction is emitted. This means that
  //!     internally an extra step will be performed to make sure that the instruction is correct. This is needed,
  //!     because by default assemblers prefer speed over strictness.
  //!
  //!     This option should be used in debug builds as it's pretty expensive.
  //!
  //!   - Architecture specific builder or compiler is used. In this case the user can turn on
  //!     \ref DiagnosticOptions::kValidateIntermediate option that adds explicit validation step before the Builder
  //!     or Compiler creates an \ref InstNode to represent an emitted instruction. Error will be returned if the
  //!     instruction is ill-formed. In addition, also \ref DiagnosticOptions::kValidateAssembler can be used, which
  //!     would not be consumed by Builder / Compiler directly, but it would be propagated to an architecture specific
  //!     \ref BaseAssembler implementation it creates during \ref BaseEmitter::finalize().
  ASMJIT_API void addDiagnosticOptions(DiagnosticOptions options) noexcept;

  //! Deactivates the given validation `options`.
  //!
  //! See \ref addDiagnosticOptions() and \ref DiagnosticOptions for more details.
  ASMJIT_API void clearDiagnosticOptions(DiagnosticOptions options) noexcept;

  //! \}

  //! \name Instruction Options
  //! \{

  //! Returns forced instruction options.
  //!
  //! Forced instruction options are merged with next instruction options before the instruction is encoded. These
  //! options have some bits reserved that are used by error handling, logging, and instruction validation purposes.
  //! Other options are globals that affect each instruction.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG InstOptions forcedInstOptions() const noexcept { return _forcedInstOptions; }

  //! Returns options of the next instruction.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG InstOptions instOptions() const noexcept { return _instOptions; }

  //! Returns options of the next instruction.
  ASMJIT_INLINE_NODEBUG void setInstOptions(InstOptions options) noexcept { _instOptions = options; }

  //! Adds options of the next instruction.
  ASMJIT_INLINE_NODEBUG void addInstOptions(InstOptions options) noexcept { _instOptions |= options; }

  //! Resets options of the next instruction.
  ASMJIT_INLINE_NODEBUG void resetInstOptions() noexcept { _instOptions = InstOptions::kNone; }

  //! Tests whether the extra register operand is valid.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool hasExtraReg() const noexcept { return _extraReg.isReg(); }

  //! Returns an extra operand that will be used by the next instruction (architecture specific).
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG const RegOnly& extraReg() const noexcept { return _extraReg; }

  //! Sets an extra operand that will be used by the next instruction (architecture specific).
  ASMJIT_INLINE_NODEBUG void setExtraReg(const Reg& reg) noexcept { _extraReg.init(reg); }

  //! Sets an extra operand that will be used by the next instruction (architecture specific).
  ASMJIT_INLINE_NODEBUG void setExtraReg(const RegOnly& reg) noexcept { _extraReg.init(reg); }

  //! Resets an extra operand that will be used by the next instruction (architecture specific).
  ASMJIT_INLINE_NODEBUG void resetExtraReg() noexcept { _extraReg.reset(); }

  //! Returns comment/annotation of the next instruction.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG const char* inlineComment() const noexcept { return _inlineComment; }

  //! Sets comment/annotation of the next instruction.
  //!
  //! \note This string is set back to null by `_emit()`, but until that it has to remain valid as the Emitter is not
  //! required to make a copy of it (and it would be slow to do that for each instruction).
  ASMJIT_INLINE_NODEBUG void setInlineComment(const char* s) noexcept { _inlineComment = s; }

  //! Resets the comment/annotation to nullptr.
  ASMJIT_INLINE_NODEBUG void resetInlineComment() noexcept { _inlineComment = nullptr; }

  //! \}

  //! \name Emitter State
  //! \{

  //! Resets the emitter state, which contains instruction options, extra register, and inline comment.
  //!
  //! Emitter can have a state that describes instruction options and extra register used by the instruction. Most
  //! instructions don't need nor use the state, however, if an instruction uses a prefix such as REX or REP prefix,
  //! which is set explicitly, then the state would contain it. This allows to mimic the syntax of assemblers such
  //! as X86. For example `rep().movs(...)` would map to a `REP MOVS` instuction on X86. The same applies to various
  //! hints and the use of a mask register in AVX-512 mode.
  ASMJIT_INLINE_NODEBUG void resetState() noexcept {
    resetInstOptions();
    resetExtraReg();
    resetInlineComment();
  }

  //! \cond INTERNAL

  //! Grabs the current emitter state and resets the emitter state at the same time, returning the state the emitter
  //! had before the state was reset.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG State _grabState() noexcept {
    State s{_instOptions | _forcedInstOptions, _extraReg, _inlineComment};
    resetState();
    return s;
  }
  //! \endcond

  //! \}

  //! \name Sections
  //! \{

  //! Switches the given `section`.
  //!
  //! Once switched, everything is added to the given `section`.
  ASMJIT_API virtual Error section(Section* section);

  //! \}

  //! \name Labels
  //! \{

  //! Creates a new label.
  [[nodiscard]]
  ASMJIT_API virtual Label newLabel();

  //! Creates a new named label.
  [[nodiscard]]
  ASMJIT_API virtual Label newNamedLabel(const char* name, size_t nameSize = SIZE_MAX, LabelType type = LabelType::kGlobal, uint32_t parentId = Globals::kInvalidId);

  //! Creates a new anonymous label with a name, which can only be used for debugging purposes.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG Label newAnonymousLabel(const char* name, size_t nameSize = SIZE_MAX) { return newNamedLabel(name, nameSize, LabelType::kAnonymous); }

  //! Creates a new external label.
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG Label newExternalLabel(const char* name, size_t nameSize = SIZE_MAX) { return newNamedLabel(name, nameSize, LabelType::kExternal); }

  //! Returns `Label` by `name`.
  //!
  //! Returns invalid Label in case that the name is invalid or label was not found.
  //!
  //! \note This function doesn't trigger ErrorHandler in case the name is invalid or no such label exist. You must
  //! always check the validity of the `Label` returned.
  [[nodiscard]]
  ASMJIT_API Label labelByName(const char* name, size_t nameSize = SIZE_MAX, uint32_t parentId = Globals::kInvalidId) noexcept;

  //! Binds the `label` to the current position of the current section.
  //!
  //! \note Attempt to bind the same label multiple times will return an error.
  ASMJIT_API virtual Error bind(const Label& label);

  //! Tests whether the label `id` is valid (i.e. registered).
  [[nodiscard]]
  ASMJIT_API bool isLabelValid(uint32_t labelId) const noexcept;

  //! Tests whether the `label` is valid (i.e. registered).
  [[nodiscard]]
  ASMJIT_INLINE_NODEBUG bool isLabelValid(const Label& label) const noexcept { return isLabelValid(label.id()); }

  //! \}

  //! \name Emit
  //! \{

  // NOTE: These `emit()` helpers are designed to address a code-bloat generated by C++ compilers to call a function
  // having many arguments. Each parameter to `_emit()` requires some code to pass it, which means that if we default
  // to 5 arguments in `_emit()` and instId the C++ compiler would have to generate a virtual function call having 5
  // parameters and additional `this` argument, which is quite a lot. Since by default most instructions have 2 to 3
  // operands it's better to introduce helpers that pass from 0 to 6 operands that help to reduce the size of emit(...)
  // function call.

  //! Emits an instruction (internal).
  ASMJIT_API Error _emitI(InstId instId);
  //! \overload
  ASMJIT_API Error _emitI(InstId instId, const Operand_& o0);
  //! \overload
  ASMJIT_API Error _emitI(InstId instId, const Operand_& o0, const Operand_& o1);
  //! \overload
  ASMJIT_API Error _emitI(InstId instId, const Operand_& o0, const Operand_& o1, const Operand_& o2);
  //! \overload
  ASMJIT_API Error _emitI(InstId instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3);
  //! \overload
  ASMJIT_API Error _emitI(InstId instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3, const Operand_& o4);
  //! \overload
  ASMJIT_API Error _emitI(InstId instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3, const Operand_& o4, const Operand_& o5);

  //! Emits an instruction `instId` with the given `operands`.
  //!
  //! This is the most universal way of emitting code, which accepts an instruction identifier and instruction
  //! operands. This is called an "unchecked" API as emit doesn't provide any type checks at compile-time. This
  //! allows to emit instruction with just \ref Operand instances, which could be handy in some cases - for
  //! example emitting generic code where you don't know whether some operand is register, memory, or immediate.
  template<typename... Args>
  ASMJIT_INLINE_NODEBUG Error emit(InstId instId, Args&&... operands) {
    return _emitI(instId, Support::ForwardOp<Args>::forward(operands)...);
  }

  //! Similar to \ref emit(), but uses array of `operands` instead.
  ASMJIT_INLINE_NODEBUG Error emitOpArray(InstId instId, const Operand_* operands, size_t opCount) {
    return _emitOpArray(instId, operands, opCount);
  }

  //! Similar to \ref emit(), but emits instruction with both instruction options and extra register, followed
  //! by an array of `operands`.
  ASMJIT_INLINE Error emitInst(const BaseInst& inst, const Operand_* operands, size_t opCount) {
    setInstOptions(inst.options());
    setExtraReg(inst.extraReg());
    return _emitOpArray(inst.id(), operands, opCount);
  }

  //! \}

  //! \cond INTERNAL
  //! \name Emit Internals
  //! \{

  //! Emits an instruction - all 6 operands must be defined.
  ASMJIT_API virtual Error _emit(InstId instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_* oExt);
  //! Emits instruction having operands stored in array.
  ASMJIT_API virtual Error _emitOpArray(InstId instId, const Operand_* operands, size_t opCount);

  //! \}
  //! \endcond

  //! \name Emit Utilities
  //! \{

  //! Emits a function prolog described by the given function `frame`.
  ASMJIT_API Error emitProlog(const FuncFrame& frame);
  //! Emits a function epilog described by the given function `frame`.
  ASMJIT_API Error emitEpilog(const FuncFrame& frame);
  //! Emits code that reassigns function `frame` arguments to the given `args`.
  ASMJIT_API Error emitArgsAssignment(const FuncFrame& frame, const FuncArgsAssignment& args);

  //! \}

  //! \name Align
  //! \{

  //! Aligns the current CodeBuffer position to the `alignment` specified.
  //!
  //! The sequence that is used to fill the gap between the aligned location and the current location depends on the
  //! align `mode`, see \ref AlignMode. The `alignment` argument specifies alignment in bytes, so for example when
  //! it's `32` it means that the code buffer will be aligned to `32` bytes.
  ASMJIT_API virtual Error align(AlignMode alignMode, uint32_t alignment);

  //! \}

  //! \name Embed
  //! \{

  //! Embeds raw data into the \ref CodeBuffer.
  ASMJIT_API virtual Error embed(const void* data, size_t dataSize);

  //! Embeds a typed data array.
  //!
  //! This is the most flexible function for embedding data as it allows to:
  //!
  //!   - Assign a `typeId` to the data, so the emitter knows the type of items stored in `data`. Binary data should
  //!     use \ref TypeId::kUInt8.
  //!
  //!   - Repeat the given data `repeatCount` times, so the data can be used as a fill pattern for example, or as a
  //!     pattern used by SIMD instructions.
  ASMJIT_API virtual Error embedDataArray(TypeId typeId, const void* data, size_t itemCount, size_t repeatCount = 1);

  //! Embeds int8_t `value` repeated by `repeatCount`.
  ASMJIT_INLINE_NODEBUG Error embedInt8(int8_t value, size_t repeatCount = 1) { return embedDataArray(TypeId::kInt8, &value, 1, repeatCount); }
  //! Embeds uint8_t `value` repeated by `repeatCount`.
  ASMJIT_INLINE_NODEBUG Error embedUInt8(uint8_t value, size_t repeatCount = 1) { return embedDataArray(TypeId::kUInt8, &value, 1, repeatCount); }
  //! Embeds int16_t `value` repeated by `repeatCount`.
  ASMJIT_INLINE_NODEBUG Error embedInt16(int16_t value, size_t repeatCount = 1) { return embedDataArray(TypeId::kInt16, &value, 1, repeatCount); }
  //! Embeds uint16_t `value` repeated by `repeatCount`.
  ASMJIT_INLINE_NODEBUG Error embedUInt16(uint16_t value, size_t repeatCount = 1) { return embedDataArray(TypeId::kUInt16, &value, 1, repeatCount); }
  //! Embeds int32_t `value` repeated by `repeatCount`.
  ASMJIT_INLINE_NODEBUG Error embedInt32(int32_t value, size_t repeatCount = 1) { return embedDataArray(TypeId::kInt32, &value, 1, repeatCount); }
  //! Embeds uint32_t `value` repeated by `repeatCount`.
  ASMJIT_INLINE_NODEBUG Error embedUInt32(uint32_t value, size_t repeatCount = 1) { return embedDataArray(TypeId::kUInt32, &value, 1, repeatCount); }
  //! Embeds int64_t `value` repeated by `repeatCount`.
  ASMJIT_INLINE_NODEBUG Error embedInt64(int64_t value, size_t repeatCount = 1) { return embedDataArray(TypeId::kInt64, &value, 1, repeatCount); }
  //! Embeds uint64_t `value` repeated by `repeatCount`.
  ASMJIT_INLINE_NODEBUG Error embedUInt64(uint64_t value, size_t repeatCount = 1) { return embedDataArray(TypeId::kUInt64, &value, 1, repeatCount); }
  //! Embeds a floating point `value` repeated by `repeatCount`.
  ASMJIT_INLINE_NODEBUG Error embedFloat(float value, size_t repeatCount = 1) { return embedDataArray(TypeId(TypeUtils::TypeIdOfT<float>::kTypeId), &value, 1, repeatCount); }
  //! Embeds a floating point `value` repeated by `repeatCount`.
  ASMJIT_INLINE_NODEBUG Error embedDouble(double value, size_t repeatCount = 1) { return embedDataArray(TypeId(TypeUtils::TypeIdOfT<double>::kTypeId), &value, 1, repeatCount); }

  //! Embeds a constant pool at the current offset by performing the following:
  //!   1. Aligns by using AlignMode::kData to the minimum `pool` alignment.
  //!   2. Binds the ConstPool label so it's bound to an aligned location.
  //!   3. Emits ConstPool content.
  ASMJIT_API virtual Error embedConstPool(const Label& label, const ConstPool& pool);

  //! Embeds an absolute `label` address as data.
  //!
  //! The `dataSize` is an optional argument that can be used to specify the size of the address data. If it's zero
  //! (default) the address size is deduced from the target architecture (either 4 or 8 bytes).
  ASMJIT_API virtual Error embedLabel(const Label& label, size_t dataSize = 0);

  //! Embeds a delta (distance) between the `label` and `base` calculating it as `label - base`. This function was
  //! designed to make it easier to embed lookup tables where each index is a relative distance of two labels.
  ASMJIT_API virtual Error embedLabelDelta(const Label& label, const Label& base, size_t dataSize = 0);

  //! \}

  //! \name Comment
  //! \{

  //! Emits a comment stored in `data` with an optional `size` parameter.
  ASMJIT_API virtual Error comment(const char* data, size_t size = SIZE_MAX);

  //! Emits a formatted comment specified by `fmt` and variable number of arguments.
  ASMJIT_API Error commentf(const char* fmt, ...);
  //! Emits a formatted comment specified by `fmt` and `ap`.
  ASMJIT_API Error commentv(const char* fmt, va_list ap);

  //! \}

  //! \name Events
  //! \{

  //! Called after the emitter was attached to `CodeHolder`.
  ASMJIT_API virtual Error onAttach(CodeHolder& code) noexcept;

  //! Called after the emitter was detached from `CodeHolder`.
  ASMJIT_API virtual Error onDetach(CodeHolder& code) noexcept;

  //! Called when CodeHolder is reinitialized when the emitter is attached.
  ASMJIT_API virtual Error onReinit(CodeHolder& code) noexcept;

  //! Called when \ref CodeHolder has updated an important setting, which involves the following:
  //!
  //!   - \ref Logger has been changed (\ref CodeHolder::setLogger() has been called).
  //!
  //!   - \ref ErrorHandler has been changed (\ref CodeHolder::setErrorHandler() has been called).
  //!
  //! This function ensures that the settings are properly propagated from \ref CodeHolder to the emitter.
  //!
  //! \note This function is virtual and can be overridden, however, if you do so, always call \ref
  //! BaseEmitter::onSettingsUpdated() within your own implementation to ensure that the emitter is
  //! in a consistent state.
  ASMJIT_API virtual void onSettingsUpdated() noexcept;

  //! \}
};

//! \}

ASMJIT_END_NAMESPACE

#endif // ASMJIT_CORE_EMITTER_H_INCLUDED
