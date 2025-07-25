// smol-v - public domain - https://github.com/aras-p/smol-v
// authored 2016-2024 by Aras Pranckevicius
// no warranty implied; use at your own risk
// See end of file for license information.
//

//
// July 2025 / Ossi Luoto
// 
// - Reworked for spirvcruncher to enable dynamic building of source files
// - Removed all failsafes and guarantees from decode paths in favor for size optimization 
//

#include "smolv.h"
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>

// --------------------------------------------------------------------------------------------
// Metadata about known SPIR-V operations

enum SpvOp
{
	SpvOpNop = 0,
	SpvOpUndef = 1,
	SpvOpSourceContinued = 2,
	SpvOpSource = 3,
	SpvOpSourceExtension = 4,
	SpvOpName = 5,
	SpvOpMemberName = 6,
	SpvOpString = 7,
	SpvOpLine = 8,
	SpvOpExtension = 10,
	SpvOpExtInstImport = 11,
	SpvOpExtInst = 12,
	SpvOpVectorShuffleCompact = 13, // not in SPIR-V, added for SMOL-V!
	SpvOpMemoryModel = 14,
	SpvOpEntryPoint = 15,
	SpvOpExecutionMode = 16,
	SpvOpCapability = 17,
	SpvOpTypeVoid = 19,
	SpvOpTypeBool = 20,
	SpvOpTypeInt = 21,
	SpvOpTypeFloat = 22,
	SpvOpTypeVector = 23,
	SpvOpTypeMatrix = 24,
	SpvOpTypeImage = 25,
	SpvOpTypeSampler = 26,
	SpvOpTypeSampledImage = 27,
	SpvOpTypeArray = 28,
	SpvOpTypeRuntimeArray = 29,
	SpvOpTypeStruct = 30,
	SpvOpTypeOpaque = 31,
	SpvOpTypePointer = 32,
	SpvOpTypeFunction = 33,
	SpvOpTypeEvent = 34,
	SpvOpTypeDeviceEvent = 35,
	SpvOpTypeReserveId = 36,
	SpvOpTypeQueue = 37,
	SpvOpTypePipe = 38,
	SpvOpTypeForwardPointer = 39,
	SpvOpConstantTrue = 41,
	SpvOpConstantFalse = 42,
	SpvOpConstant = 43,
	SpvOpConstantComposite = 44,
	SpvOpConstantSampler = 45,
	SpvOpConstantNull = 46,
	SpvOpSpecConstantTrue = 48,
	SpvOpSpecConstantFalse = 49,
	SpvOpSpecConstant = 50,
	SpvOpSpecConstantComposite = 51,
	SpvOpSpecConstantOp = 52,
	SpvOpFunction = 54,
	SpvOpFunctionParameter = 55,
	SpvOpFunctionEnd = 56,
	SpvOpFunctionCall = 57,
	SpvOpVariable = 59,
	SpvOpImageTexelPointer = 60,
	SpvOpLoad = 61,
	SpvOpStore = 62,
	SpvOpCopyMemory = 63,
	SpvOpCopyMemorySized = 64,
	SpvOpAccessChain = 65,
	SpvOpInBoundsAccessChain = 66,
	SpvOpPtrAccessChain = 67,
	SpvOpArrayLength = 68,
	SpvOpGenericPtrMemSemantics = 69,
	SpvOpInBoundsPtrAccessChain = 70,
	SpvOpDecorate = 71,
	SpvOpMemberDecorate = 72,
	SpvOpDecorationGroup = 73,
	SpvOpGroupDecorate = 74,
	SpvOpGroupMemberDecorate = 75,
	SpvOpVectorExtractDynamic = 77,
	SpvOpVectorInsertDynamic = 78,
	SpvOpVectorShuffle = 79,
	SpvOpCompositeConstruct = 80,
	SpvOpCompositeExtract = 81,
	SpvOpCompositeInsert = 82,
	SpvOpCopyObject = 83,
	SpvOpTranspose = 84,
	SpvOpSampledImage = 86,
	SpvOpImageSampleImplicitLod = 87,
	SpvOpImageSampleExplicitLod = 88,
	SpvOpImageSampleDrefImplicitLod = 89,
	SpvOpImageSampleDrefExplicitLod = 90,
	SpvOpImageSampleProjImplicitLod = 91,
	SpvOpImageSampleProjExplicitLod = 92,
	SpvOpImageSampleProjDrefImplicitLod = 93,
	SpvOpImageSampleProjDrefExplicitLod = 94,
	SpvOpImageFetch = 95,
	SpvOpImageGather = 96,
	SpvOpImageDrefGather = 97,
	SpvOpImageRead = 98,
	SpvOpImageWrite = 99,
	SpvOpImage = 100,
	SpvOpImageQueryFormat = 101,
	SpvOpImageQueryOrder = 102,
	SpvOpImageQuerySizeLod = 103,
	SpvOpImageQuerySize = 104,
	SpvOpImageQueryLod = 105,
	SpvOpImageQueryLevels = 106,
	SpvOpImageQuerySamples = 107,
	SpvOpConvertFToU = 109,
	SpvOpConvertFToS = 110,
	SpvOpConvertSToF = 111,
	SpvOpConvertUToF = 112,
	SpvOpUConvert = 113,
	SpvOpSConvert = 114,
	SpvOpFConvert = 115,
	SpvOpQuantizeToF16 = 116,
	SpvOpConvertPtrToU = 117,
	SpvOpSatConvertSToU = 118,
	SpvOpSatConvertUToS = 119,
	SpvOpConvertUToPtr = 120,
	SpvOpPtrCastToGeneric = 121,
	SpvOpGenericCastToPtr = 122,
	SpvOpGenericCastToPtrExplicit = 123,
	SpvOpBitcast = 124,
	SpvOpSNegate = 126,
	SpvOpFNegate = 127,
	SpvOpIAdd = 128,
	SpvOpFAdd = 129,
	SpvOpISub = 130,
	SpvOpFSub = 131,
	SpvOpIMul = 132,
	SpvOpFMul = 133,
	SpvOpUDiv = 134,
	SpvOpSDiv = 135,
	SpvOpFDiv = 136,
	SpvOpUMod = 137,
	SpvOpSRem = 138,
	SpvOpSMod = 139,
	SpvOpFRem = 140,
	SpvOpFMod = 141,
	SpvOpVectorTimesScalar = 142,
	SpvOpMatrixTimesScalar = 143,
	SpvOpVectorTimesMatrix = 144,
	SpvOpMatrixTimesVector = 145,
	SpvOpMatrixTimesMatrix = 146,
	SpvOpOuterProduct = 147,
	SpvOpDot = 148,
	SpvOpIAddCarry = 149,
	SpvOpISubBorrow = 150,
	SpvOpUMulExtended = 151,
	SpvOpSMulExtended = 152,
	SpvOpAny = 154,
	SpvOpAll = 155,
	SpvOpIsNan = 156,
	SpvOpIsInf = 157,
	SpvOpIsFinite = 158,
	SpvOpIsNormal = 159,
	SpvOpSignBitSet = 160,
	SpvOpLessOrGreater = 161,
	SpvOpOrdered = 162,
	SpvOpUnordered = 163,
	SpvOpLogicalEqual = 164,
	SpvOpLogicalNotEqual = 165,
	SpvOpLogicalOr = 166,
	SpvOpLogicalAnd = 167,
	SpvOpLogicalNot = 168,
	SpvOpSelect = 169,
	SpvOpIEqual = 170,
	SpvOpINotEqual = 171,
	SpvOpUGreaterThan = 172,
	SpvOpSGreaterThan = 173,
	SpvOpUGreaterThanEqual = 174,
	SpvOpSGreaterThanEqual = 175,
	SpvOpULessThan = 176,
	SpvOpSLessThan = 177,
	SpvOpULessThanEqual = 178,
	SpvOpSLessThanEqual = 179,
	SpvOpFOrdEqual = 180,
	SpvOpFUnordEqual = 181,
	SpvOpFOrdNotEqual = 182,
	SpvOpFUnordNotEqual = 183,
	SpvOpFOrdLessThan = 184,
	SpvOpFUnordLessThan = 185,
	SpvOpFOrdGreaterThan = 186,
	SpvOpFUnordGreaterThan = 187,
	SpvOpFOrdLessThanEqual = 188,
	SpvOpFUnordLessThanEqual = 189,
	SpvOpFOrdGreaterThanEqual = 190,
	SpvOpFUnordGreaterThanEqual = 191,
	SpvOpShiftRightLogical = 194,
	SpvOpShiftRightArithmetic = 195,
	SpvOpShiftLeftLogical = 196,
	SpvOpBitwiseOr = 197,
	SpvOpBitwiseXor = 198,
	SpvOpBitwiseAnd = 199,
	SpvOpNot = 200,
	SpvOpBitFieldInsert = 201,
	SpvOpBitFieldSExtract = 202,
	SpvOpBitFieldUExtract = 203,
	SpvOpBitReverse = 204,
	SpvOpBitCount = 205,
	SpvOpDPdx = 207,
	SpvOpDPdy = 208,
	SpvOpFwidth = 209,
	SpvOpDPdxFine = 210,
	SpvOpDPdyFine = 211,
	SpvOpFwidthFine = 212,
	SpvOpDPdxCoarse = 213,
	SpvOpDPdyCoarse = 214,
	SpvOpFwidthCoarse = 215,
	SpvOpEmitVertex = 218,
	SpvOpEndPrimitive = 219,
	SpvOpEmitStreamVertex = 220,
	SpvOpEndStreamPrimitive = 221,
	SpvOpControlBarrier = 224,
	SpvOpMemoryBarrier = 225,
	SpvOpAtomicLoad = 227,
	SpvOpAtomicStore = 228,
	SpvOpAtomicExchange = 229,
	SpvOpAtomicCompareExchange = 230,
	SpvOpAtomicCompareExchangeWeak = 231,
	SpvOpAtomicIIncrement = 232,
	SpvOpAtomicIDecrement = 233,
	SpvOpAtomicIAdd = 234,
	SpvOpAtomicISub = 235,
	SpvOpAtomicSMin = 236,
	SpvOpAtomicUMin = 237,
	SpvOpAtomicSMax = 238,
	SpvOpAtomicUMax = 239,
	SpvOpAtomicAnd = 240,
	SpvOpAtomicOr = 241,
	SpvOpAtomicXor = 242,
	SpvOpPhi = 245,
	SpvOpLoopMerge = 246,
	SpvOpSelectionMerge = 247,
	SpvOpLabel = 248,
	SpvOpBranch = 249,
	SpvOpBranchConditional = 250,
	SpvOpSwitch = 251,
	SpvOpKill = 252,
	SpvOpReturn = 253,
	SpvOpReturnValue = 254,
	SpvOpUnreachable = 255,
	SpvOpLifetimeStart = 256,
	SpvOpLifetimeStop = 257,
	SpvOpGroupAsyncCopy = 259,
	SpvOpGroupWaitEvents = 260,
	SpvOpGroupAll = 261,
	SpvOpGroupAny = 262,
	SpvOpGroupBroadcast = 263,
	SpvOpGroupIAdd = 264,
	SpvOpGroupFAdd = 265,
	SpvOpGroupFMin = 266,
	SpvOpGroupUMin = 267,
	SpvOpGroupSMin = 268,
	SpvOpGroupFMax = 269,
	SpvOpGroupUMax = 270,
	SpvOpGroupSMax = 271,
	SpvOpReadPipe = 274,
	SpvOpWritePipe = 275,
	SpvOpReservedReadPipe = 276,
	SpvOpReservedWritePipe = 277,
	SpvOpReserveReadPipePackets = 278,
	SpvOpReserveWritePipePackets = 279,
	SpvOpCommitReadPipe = 280,
	SpvOpCommitWritePipe = 281,
	SpvOpIsValidReserveId = 282,
	SpvOpGetNumPipePackets = 283,
	SpvOpGetMaxPipePackets = 284,
	SpvOpGroupReserveReadPipePackets = 285,
	SpvOpGroupReserveWritePipePackets = 286,
	SpvOpGroupCommitReadPipe = 287,
	SpvOpGroupCommitWritePipe = 288,
	SpvOpEnqueueMarker = 291,
	SpvOpEnqueueKernel = 292,
	SpvOpGetKernelNDrangeSubGroupCount = 293,
	SpvOpGetKernelNDrangeMaxSubGroupSize = 294,
	SpvOpGetKernelWorkGroupSize = 295,
	SpvOpGetKernelPreferredWorkGroupSizeMultiple = 296,
	SpvOpRetainEvent = 297,
	SpvOpReleaseEvent = 298,
	SpvOpCreateUserEvent = 299,
	SpvOpIsValidEvent = 300,
	SpvOpSetUserEventStatus = 301,
	SpvOpCaptureEventProfilingInfo = 302,
	SpvOpGetDefaultQueue = 303,
	SpvOpBuildNDRange = 304,
	SpvOpImageSparseSampleImplicitLod = 305,
	SpvOpImageSparseSampleExplicitLod = 306,
	SpvOpImageSparseSampleDrefImplicitLod = 307,
	SpvOpImageSparseSampleDrefExplicitLod = 308,
	SpvOpImageSparseSampleProjImplicitLod = 309,
	SpvOpImageSparseSampleProjExplicitLod = 310,
	SpvOpImageSparseSampleProjDrefImplicitLod = 311,
	SpvOpImageSparseSampleProjDrefExplicitLod = 312,
	SpvOpImageSparseFetch = 313,
	SpvOpImageSparseGather = 314,
	SpvOpImageSparseDrefGather = 315,
	SpvOpImageSparseTexelsResident = 316,
	SpvOpNoLine = 317,
	SpvOpAtomicFlagTestAndSet = 318,
	SpvOpAtomicFlagClear = 319,
	SpvOpImageSparseRead = 320,
	SpvOpSizeOf = 321,
	SpvOpTypePipeStorage = 322,
	SpvOpConstantPipeStorage = 323,
	SpvOpCreatePipeFromPipeStorage = 324,
	SpvOpGetKernelLocalSizeForSubgroupCount = 325,
	SpvOpGetKernelMaxNumSubgroups = 326,
	SpvOpTypeNamedBarrier = 327,
	SpvOpNamedBarrierInitialize = 328,
	SpvOpMemoryNamedBarrier = 329,
	SpvOpModuleProcessed = 330,
	SpvOpExecutionModeId = 331,
	SpvOpDecorateId = 332,
	SpvOpGroupNonUniformElect = 333,
	SpvOpGroupNonUniformAll = 334,
	SpvOpGroupNonUniformAny = 335,
	SpvOpGroupNonUniformAllEqual = 336,
	SpvOpGroupNonUniformBroadcast = 337,
	SpvOpGroupNonUniformBroadcastFirst = 338,
	SpvOpGroupNonUniformBallot = 339,
	SpvOpGroupNonUniformInverseBallot = 340,
	SpvOpGroupNonUniformBallotBitExtract = 341,
	SpvOpGroupNonUniformBallotBitCount = 342,
	SpvOpGroupNonUniformBallotFindLSB = 343,
	SpvOpGroupNonUniformBallotFindMSB = 344,
	SpvOpGroupNonUniformShuffle = 345,
	SpvOpGroupNonUniformShuffleXor = 346,
	SpvOpGroupNonUniformShuffleUp = 347,
	SpvOpGroupNonUniformShuffleDown = 348,
	SpvOpGroupNonUniformIAdd = 349,
	SpvOpGroupNonUniformFAdd = 350,
	SpvOpGroupNonUniformIMul = 351,
	SpvOpGroupNonUniformFMul = 352,
	SpvOpGroupNonUniformSMin = 353,
	SpvOpGroupNonUniformUMin = 354,
	SpvOpGroupNonUniformFMin = 355,
	SpvOpGroupNonUniformSMax = 356,
	SpvOpGroupNonUniformUMax = 357,
	SpvOpGroupNonUniformFMax = 358,
	SpvOpGroupNonUniformBitwiseAnd = 359,
	SpvOpGroupNonUniformBitwiseOr = 360,
	SpvOpGroupNonUniformBitwiseXor = 361,
	SpvOpGroupNonUniformLogicalAnd = 362,
	SpvOpGroupNonUniformLogicalOr = 363,
	SpvOpGroupNonUniformLogicalXor = 364,
	SpvOpGroupNonUniformQuadBroadcast = 365,
	SpvOpGroupNonUniformQuadSwap = 366,
};
static const int kKnownOpsCount = SpvOpGroupNonUniformQuadSwap + 1;

struct OpData
{
	uint8_t hasResult;	// does it have result ID?
	uint8_t hasType;	// does it have type ID?
	uint8_t deltaFromResult; // How many words after (optional) type+result to write out as deltas from result?
	uint8_t varrest;	// should the rest of words be written in varint encoding?
};
static const OpData kSpirvOpData[] =
{
// >>>>> SPIRVCRUNCHER Spv Start >>>>>
	{0, 0, 0, 0}, // Nop
	{1, 1, 0, 0}, // Undef
	{0, 0, 0, 0}, // SourceContinued
	{0, 0, 0, 1}, // Source
	{0, 0, 0, 0}, // SourceExtension
	{0, 0, 0, 0}, // Name
	{0, 0, 0, 0}, // MemberName
	{0, 0, 0, 0}, // String
	{0, 0, 0, 1}, // Line
	{1, 1, 0, 0}, // #9
	{0, 0, 0, 0}, // Extension
	{1, 0, 0, 0}, // ExtInstImport
	{1, 1, 0, 1}, // ExtInst
	{1, 1, 2, 1}, // VectorShuffleCompact - new in SMOLV
	{0, 0, 0, 1}, // MemoryModel
	{0, 0, 0, 1}, // EntryPoint
	{0, 0, 0, 1}, // ExecutionMode
	{0, 0, 0, 1}, // Capability
	{1, 1, 0, 0}, // #18
	{1, 0, 0, 1}, // TypeVoid
	{1, 0, 0, 1}, // TypeBool
	{1, 0, 0, 1}, // TypeInt
	{1, 0, 0, 1}, // TypeFloat
	{1, 0, 0, 1}, // TypeVector
	{1, 0, 0, 1}, // TypeMatrix
	{1, 0, 0, 1}, // TypeImage
	{1, 0, 0, 1}, // TypeSampler
	{1, 0, 0, 1}, // TypeSampledImage
	{1, 0, 0, 1}, // TypeArray
	{1, 0, 0, 1}, // TypeRuntimeArray
	{1, 0, 0, 1}, // TypeStruct
	{1, 0, 0, 1}, // TypeOpaque
	{1, 0, 0, 1}, // TypePointer
	{1, 0, 0, 1}, // TypeFunction
	{1, 0, 0, 1}, // TypeEvent
	{1, 0, 0, 1}, // TypeDeviceEvent
	{1, 0, 0, 1}, // TypeReserveId
	{1, 0, 0, 1}, // TypeQueue
	{1, 0, 0, 1}, // TypePipe
	{0, 0, 0, 1}, // TypeForwardPointer
	{1, 1, 0, 0}, // #40
	{1, 1, 0, 0}, // ConstantTrue
	{1, 1, 0, 0}, // ConstantFalse
	{1, 1, 0, 0}, // Constant
	{1, 1, 9, 0}, // ConstantComposite
	{1, 1, 0, 1}, // ConstantSampler
	{1, 1, 0, 0}, // ConstantNull
	{1, 1, 0, 0}, // #47
	{1, 1, 0, 0}, // SpecConstantTrue
	{1, 1, 0, 0}, // SpecConstantFalse
	{1, 1, 0, 0}, // SpecConstant
	{1, 1, 9, 0}, // SpecConstantComposite
	{1, 1, 0, 0}, // SpecConstantOp
	{1, 1, 0, 0}, // #53
	{1, 1, 0, 1}, // Function
	{1, 1, 0, 0}, // FunctionParameter
	{0, 0, 0, 0}, // FunctionEnd
	{1, 1, 9, 0}, // FunctionCall
	{1, 1, 0, 0}, // #58
	{1, 1, 0, 1}, // Variable
	{1, 1, 0, 0}, // ImageTexelPointer
	{1, 1, 1, 1}, // Load
	{0, 0, 2, 1}, // Store
	{0, 0, 0, 0}, // CopyMemory
	{0, 0, 0, 0}, // CopyMemorySized
	{1, 1, 0, 1}, // AccessChain
	{1, 1, 0, 0}, // InBoundsAccessChain
	{1, 1, 0, 0}, // PtrAccessChain
	{1, 1, 0, 0}, // ArrayLength
	{1, 1, 0, 0}, // GenericPtrMemSemantics
	{1, 1, 0, 0}, // InBoundsPtrAccessChain
	{0, 0, 0, 1}, // Decorate
	{0, 0, 0, 1}, // MemberDecorate
	{1, 0, 0, 0}, // DecorationGroup
	{0, 0, 0, 0}, // GroupDecorate
	{0, 0, 0, 0}, // GroupMemberDecorate
	{1, 1, 0, 0}, // #76
	{1, 1, 1, 1}, // VectorExtractDynamic
	{1, 1, 2, 1}, // VectorInsertDynamic
	{1, 1, 2, 1}, // VectorShuffle
	{1, 1, 9, 0}, // CompositeConstruct
	{1, 1, 1, 1}, // CompositeExtract
	{1, 1, 2, 1}, // CompositeInsert
	{1, 1, 1, 0}, // CopyObject
	{1, 1, 0, 0}, // Transpose
	{1, 1, 0, 0}, // #85
	{1, 1, 0, 0}, // SampledImage
	{1, 1, 2, 1}, // ImageSampleImplicitLod
	{1, 1, 2, 1}, // ImageSampleExplicitLod
	{1, 1, 3, 1}, // ImageSampleDrefImplicitLod
	{1, 1, 3, 1}, // ImageSampleDrefExplicitLod
	{1, 1, 2, 1}, // ImageSampleProjImplicitLod
	{1, 1, 2, 1}, // ImageSampleProjExplicitLod
	{1, 1, 3, 1}, // ImageSampleProjDrefImplicitLod
	{1, 1, 3, 1}, // ImageSampleProjDrefExplicitLod
	{1, 1, 2, 1}, // ImageFetch
	{1, 1, 3, 1}, // ImageGather
	{1, 1, 3, 1}, // ImageDrefGather
	{1, 1, 2, 1}, // ImageRead
	{0, 0, 3, 1}, // ImageWrite
	{1, 1, 1, 0}, // Image
	{1, 1, 1, 0}, // ImageQueryFormat
	{1, 1, 1, 0}, // ImageQueryOrder
	{1, 1, 2, 0}, // ImageQuerySizeLod
	{1, 1, 1, 0}, // ImageQuerySize
	{1, 1, 2, 0}, // ImageQueryLod
	{1, 1, 1, 0}, // ImageQueryLevels
	{1, 1, 1, 0}, // ImageQuerySamples
	{1, 1, 0, 0}, // #108
	{1, 1, 1, 0}, // ConvertFToU
	{1, 1, 1, 0}, // ConvertFToS
	{1, 1, 1, 0}, // ConvertSToF
	{1, 1, 1, 0}, // ConvertUToF
	{1, 1, 1, 0}, // UConvert
	{1, 1, 1, 0}, // SConvert
	{1, 1, 1, 0}, // FConvert
	{1, 1, 1, 0}, // QuantizeToF16
	{1, 1, 1, 0}, // ConvertPtrToU
	{1, 1, 1, 0}, // SatConvertSToU
	{1, 1, 1, 0}, // SatConvertUToS
	{1, 1, 1, 0}, // ConvertUToPtr
	{1, 1, 1, 0}, // PtrCastToGeneric
	{1, 1, 1, 0}, // GenericCastToPtr
	{1, 1, 1, 1}, // GenericCastToPtrExplicit
	{1, 1, 1, 0}, // Bitcast
	{1, 1, 0, 0}, // #125
	{1, 1, 1, 0}, // SNegate
	{1, 1, 1, 0}, // FNegate
	{1, 1, 2, 0}, // IAdd
	{1, 1, 2, 0}, // FAdd
	{1, 1, 2, 0}, // ISub
	{1, 1, 2, 0}, // FSub
	{1, 1, 2, 0}, // IMul
	{1, 1, 2, 0}, // FMul
	{1, 1, 2, 0}, // UDiv
	{1, 1, 2, 0}, // SDiv
	{1, 1, 2, 0}, // FDiv
	{1, 1, 2, 0}, // UMod
	{1, 1, 2, 0}, // SRem
	{1, 1, 2, 0}, // SMod
	{1, 1, 2, 0}, // FRem
	{1, 1, 2, 0}, // FMod
	{1, 1, 2, 0}, // VectorTimesScalar
	{1, 1, 2, 0}, // MatrixTimesScalar
	{1, 1, 2, 0}, // VectorTimesMatrix
	{1, 1, 2, 0}, // MatrixTimesVector
	{1, 1, 2, 0}, // MatrixTimesMatrix
	{1, 1, 2, 0}, // OuterProduct
	{1, 1, 2, 0}, // Dot
	{1, 1, 2, 0}, // IAddCarry
	{1, 1, 2, 0}, // ISubBorrow
	{1, 1, 2, 0}, // UMulExtended
	{1, 1, 2, 0}, // SMulExtended
	{1, 1, 0, 0}, // #153
	{1, 1, 1, 0}, // Any
	{1, 1, 1, 0}, // All
	{1, 1, 1, 0}, // IsNan
	{1, 1, 1, 0}, // IsInf
	{1, 1, 1, 0}, // IsFinite
	{1, 1, 1, 0}, // IsNormal
	{1, 1, 1, 0}, // SignBitSet
	{1, 1, 2, 0}, // LessOrGreater
	{1, 1, 2, 0}, // Ordered
	{1, 1, 2, 0}, // Unordered
	{1, 1, 2, 0}, // LogicalEqual
	{1, 1, 2, 0}, // LogicalNotEqual
	{1, 1, 2, 0}, // LogicalOr
	{1, 1, 2, 0}, // LogicalAnd
	{1, 1, 1, 0}, // LogicalNot
	{1, 1, 3, 0}, // Select
	{1, 1, 2, 0}, // IEqual
	{1, 1, 2, 0}, // INotEqual
	{1, 1, 2, 0}, // UGreaterThan
	{1, 1, 2, 0}, // SGreaterThan
	{1, 1, 2, 0}, // UGreaterThanEqual
	{1, 1, 2, 0}, // SGreaterThanEqual
	{1, 1, 2, 0}, // ULessThan
	{1, 1, 2, 0}, // SLessThan
	{1, 1, 2, 0}, // ULessThanEqual
	{1, 1, 2, 0}, // SLessThanEqual
	{1, 1, 2, 0}, // FOrdEqual
	{1, 1, 2, 0}, // FUnordEqual
	{1, 1, 2, 0}, // FOrdNotEqual
	{1, 1, 2, 0}, // FUnordNotEqual
	{1, 1, 2, 0}, // FOrdLessThan
	{1, 1, 2, 0}, // FUnordLessThan
	{1, 1, 2, 0}, // FOrdGreaterThan
	{1, 1, 2, 0}, // FUnordGreaterThan
	{1, 1, 2, 0}, // FOrdLessThanEqual
	{1, 1, 2, 0}, // FUnordLessThanEqual
	{1, 1, 2, 0}, // FOrdGreaterThanEqual
	{1, 1, 2, 0}, // FUnordGreaterThanEqual
	{1, 1, 0, 0}, // #192
	{1, 1, 0, 0}, // #193
	{1, 1, 2, 0}, // ShiftRightLogical
	{1, 1, 2, 0}, // ShiftRightArithmetic
	{1, 1, 2, 0}, // ShiftLeftLogical
	{1, 1, 2, 0}, // BitwiseOr
	{1, 1, 2, 0}, // BitwiseXor
	{1, 1, 2, 0}, // BitwiseAnd
	{1, 1, 1, 0}, // Not
	{1, 1, 4, 0}, // BitFieldInsert
	{1, 1, 3, 0}, // BitFieldSExtract
	{1, 1, 3, 0}, // BitFieldUExtract
	{1, 1, 1, 0}, // BitReverse
	{1, 1, 1, 0}, // BitCount
	{1, 1, 0, 0}, // #206
	{1, 1, 0, 0}, // DPdx
	{1, 1, 0, 0}, // DPdy
	{1, 1, 0, 0}, // Fwidth
	{1, 1, 0, 0}, // DPdxFine
	{1, 1, 0, 0}, // DPdyFine
	{1, 1, 0, 0}, // FwidthFine
	{1, 1, 0, 0}, // DPdxCoarse
	{1, 1, 0, 0}, // DPdyCoarse
	{1, 1, 0, 0}, // FwidthCoarse
	{1, 1, 0, 0}, // #216
	{1, 1, 0, 0}, // #217
	{0, 0, 0, 0}, // EmitVertex
	{0, 0, 0, 0}, // EndPrimitive
	{0, 0, 0, 0}, // EmitStreamVertex
	{0, 0, 0, 0}, // EndStreamPrimitive
	{1, 1, 0, 0}, // #222
	{1, 1, 0, 0}, // #223
	{0, 0, 3, 0}, // ControlBarrier
	{0, 0, 2, 0}, // MemoryBarrier
	{1, 1, 0, 0}, // #226
	{1, 1, 0, 0}, // AtomicLoad
	{0, 0, 0, 0}, // AtomicStore
	{1, 1, 0, 0}, // AtomicExchange
	{1, 1, 0, 0}, // AtomicCompareExchange
	{1, 1, 0, 0}, // AtomicCompareExchangeWeak
	{1, 1, 0, 0}, // AtomicIIncrement
	{1, 1, 0, 0}, // AtomicIDecrement
	{1, 1, 0, 0}, // AtomicIAdd
	{1, 1, 0, 0}, // AtomicISub
	{1, 1, 0, 0}, // AtomicSMin
	{1, 1, 0, 0}, // AtomicUMin
	{1, 1, 0, 0}, // AtomicSMax
	{1, 1, 0, 0}, // AtomicUMax
	{1, 1, 0, 0}, // AtomicAnd
	{1, 1, 0, 0}, // AtomicOr
	{1, 1, 0, 0}, // AtomicXor
	{1, 1, 0, 0}, // #243
	{1, 1, 0, 0}, // #244
	{1, 1, 0, 0}, // Phi
	{0, 0, 2, 1}, // LoopMerge
	{0, 0, 1, 1}, // SelectionMerge
	{1, 0, 0, 0}, // Label
	{0, 0, 1, 0}, // Branch
	{0, 0, 3, 1}, // BranchConditional
	{0, 0, 0, 0}, // Switch
	{0, 0, 0, 0}, // Kill
	{0, 0, 0, 0}, // Return
	{0, 0, 0, 0}, // ReturnValue
	{0, 0, 0, 0}, // Unreachable
	{0, 0, 0, 0}, // LifetimeStart
	{0, 0, 0, 0}, // LifetimeStop
	{1, 1, 0, 0}, // #258
	{1, 1, 0, 0}, // GroupAsyncCopy
	{0, 0, 0, 0}, // GroupWaitEvents
	{1, 1, 0, 0}, // GroupAll
	{1, 1, 0, 0}, // GroupAny
	{1, 1, 0, 0}, // GroupBroadcast
	{1, 1, 0, 0}, // GroupIAdd
	{1, 1, 0, 0}, // GroupFAdd
	{1, 1, 0, 0}, // GroupFMin
	{1, 1, 0, 0}, // GroupUMin
	{1, 1, 0, 0}, // GroupSMin
	{1, 1, 0, 0}, // GroupFMax
	{1, 1, 0, 0}, // GroupUMax
	{1, 1, 0, 0}, // GroupSMax
	{1, 1, 0, 0}, // #272
	{1, 1, 0, 0}, // #273
	{1, 1, 0, 0}, // ReadPipe
	{1, 1, 0, 0}, // WritePipe
	{1, 1, 0, 0}, // ReservedReadPipe
	{1, 1, 0, 0}, // ReservedWritePipe
	{1, 1, 0, 0}, // ReserveReadPipePackets
	{1, 1, 0, 0}, // ReserveWritePipePackets
	{0, 0, 0, 0}, // CommitReadPipe
	{0, 0, 0, 0}, // CommitWritePipe
	{1, 1, 0, 0}, // IsValidReserveId
	{1, 1, 0, 0}, // GetNumPipePackets
	{1, 1, 0, 0}, // GetMaxPipePackets
	{1, 1, 0, 0}, // GroupReserveReadPipePackets
	{1, 1, 0, 0}, // GroupReserveWritePipePackets
	{0, 0, 0, 0}, // GroupCommitReadPipe
	{0, 0, 0, 0}, // GroupCommitWritePipe
	{1, 1, 0, 0}, // #289
	{1, 1, 0, 0}, // #290
	{1, 1, 0, 0}, // EnqueueMarker
	{1, 1, 0, 0}, // EnqueueKernel
	{1, 1, 0, 0}, // GetKernelNDrangeSubGroupCount
	{1, 1, 0, 0}, // GetKernelNDrangeMaxSubGroupSize
	{1, 1, 0, 0}, // GetKernelWorkGroupSize
	{1, 1, 0, 0}, // GetKernelPreferredWorkGroupSizeMultiple
	{0, 0, 0, 0}, // RetainEvent
	{0, 0, 0, 0}, // ReleaseEvent
	{1, 1, 0, 0}, // CreateUserEvent
	{1, 1, 0, 0}, // IsValidEvent
	{0, 0, 0, 0}, // SetUserEventStatus
	{0, 0, 0, 0}, // CaptureEventProfilingInfo
	{1, 1, 0, 0}, // GetDefaultQueue
	{1, 1, 0, 0}, // BuildNDRange
	{1, 1, 2, 1}, // ImageSparseSampleImplicitLod
	{1, 1, 2, 1}, // ImageSparseSampleExplicitLod
	{1, 1, 3, 1}, // ImageSparseSampleDrefImplicitLod
	{1, 1, 3, 1}, // ImageSparseSampleDrefExplicitLod
	{1, 1, 2, 1}, // ImageSparseSampleProjImplicitLod
	{1, 1, 2, 1}, // ImageSparseSampleProjExplicitLod
	{1, 1, 3, 1}, // ImageSparseSampleProjDrefImplicitLod
	{1, 1, 3, 1}, // ImageSparseSampleProjDrefExplicitLod
	{1, 1, 2, 1}, // ImageSparseFetch
	{1, 1, 3, 1}, // ImageSparseGather
	{1, 1, 3, 1}, // ImageSparseDrefGather
	{1, 1, 1, 0}, // ImageSparseTexelsResident
	{0, 0, 0, 0}, // NoLine
	{1, 1, 0, 0}, // AtomicFlagTestAndSet
	{0, 0, 0, 0}, // AtomicFlagClear
	{1, 1, 0, 0}, // ImageSparseRead
	{1, 1, 0, 0}, // SizeOf
	{1, 1, 0, 0}, // TypePipeStorage
	{1, 1, 0, 0}, // ConstantPipeStorage
	{1, 1, 0, 0}, // CreatePipeFromPipeStorage
	{1, 1, 0, 0}, // GetKernelLocalSizeForSubgroupCount
	{1, 1, 0, 0}, // GetKernelMaxNumSubgroups
	{1, 1, 0, 0}, // TypeNamedBarrier
	{1, 1, 0, 1}, // NamedBarrierInitialize
	{0, 0, 2, 1}, // MemoryNamedBarrier
	{1, 1, 0, 0}, // ModuleProcessed
	{0, 0, 0, 1}, // ExecutionModeId
	{0, 0, 0, 1}, // DecorateId
	{1, 1, 1, 1}, // GroupNonUniformElect
	{1, 1, 1, 1}, // GroupNonUniformAll
	{1, 1, 1, 1}, // GroupNonUniformAny
	{1, 1, 1, 1}, // GroupNonUniformAllEqual
	{1, 1, 1, 1}, // GroupNonUniformBroadcast
	{1, 1, 1, 1}, // GroupNonUniformBroadcastFirst
	{1, 1, 1, 1}, // GroupNonUniformBallot
	{1, 1, 1, 1}, // GroupNonUniformInverseBallot
	{1, 1, 1, 1}, // GroupNonUniformBallotBitExtract
	{1, 1, 1, 1}, // GroupNonUniformBallotBitCount
	{1, 1, 1, 1}, // GroupNonUniformBallotFindLSB
	{1, 1, 1, 1}, // GroupNonUniformBallotFindMSB
	{1, 1, 1, 1}, // GroupNonUniformShuffle
	{1, 1, 1, 1}, // GroupNonUniformShuffleXor
	{1, 1, 1, 1}, // GroupNonUniformShuffleUp
	{1, 1, 1, 1}, // GroupNonUniformShuffleDown
	{1, 1, 1, 1}, // GroupNonUniformIAdd
	{1, 1, 1, 1}, // GroupNonUniformFAdd
	{1, 1, 1, 1}, // GroupNonUniformIMul
	{1, 1, 1, 1}, // GroupNonUniformFMul
	{1, 1, 1, 1}, // GroupNonUniformSMin
	{1, 1, 1, 1}, // GroupNonUniformUMin
	{1, 1, 1, 1}, // GroupNonUniformFMin
	{1, 1, 1, 1}, // GroupNonUniformSMax
	{1, 1, 1, 1}, // GroupNonUniformUMax
	{1, 1, 1, 1}, // GroupNonUniformFMax
	{1, 1, 1, 1}, // GroupNonUniformBitwiseAnd
	{1, 1, 1, 1}, // GroupNonUniformBitwiseOr
	{1, 1, 1, 1}, // GroupNonUniformBitwiseXor
	{1, 1, 1, 1}, // GroupNonUniformLogicalAnd
	{1, 1, 1, 1}, // GroupNonUniformLogicalOr
	{1, 1, 1, 1}, // GroupNonUniformLogicalXor
	{1, 1, 1, 1}, // GroupNonUniformQuadBroadcast
	{1, 1, 1, 1}, // GroupNonUniformQuadSwap
// >>>>> SPIRVCRUNCHER Spv End >>>>>
};


static bool smolv_OpHasResult(SpvOp op, int opsCount)
{
	return kSpirvOpData[op].hasResult != 0;
}

static bool smolv_OpHasType(SpvOp op, int opsCount)
{
	return kSpirvOpData[op].hasType != 0;
}

static int smolv_OpDeltaFromResult(SpvOp op, int opsCount)
{
	return kSpirvOpData[op].deltaFromResult;
}

static bool smolv_OpVarRest(SpvOp op, int opsCount)
{
	return kSpirvOpData[op].varrest != 0;
}

static int smolv_DecorationExtraOps(int dec)
{
	if (dec == 0 || (dec >= 2 && dec <= 5)) // RelaxedPrecision, Block..ColMajor
		return 0;
	if (dec >= 29 && dec <= 37) // Stream..XfbStride
		return 1;
	return -1; // unknown, encode length
}


// --------------------------------------------------------------------------------------------

static const int kSpirVHeaderMagic = 0x07230203;

inline void smolv_Write4(uint8_t*& buf, uint32_t v)
{
	memcpy(buf, &v, 4);
	buf += 4;
}

inline void smolv_Read4(const uint8_t*& data, const uint8_t* dataEnd, uint32_t& outv)
{
	outv = (data[0]) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
	data += 4;
}


// --------------------------------------------------------------------------------------------

// Variable-length integer encoding for unsigned integers. In each byte:
// - highest bit set if more bytes follow, cleared if this is last byte.
// - other 7 bits are the actual value payload.
// Takes 1-5 bytes to encode an integer (values between 0 and 127 take one byte, etc.).


static void smolv_ReadVarint(const uint8_t*& data, const uint8_t* dataEnd, uint32_t& outVal)
{
	//	uint32_t v = 0;
	outVal = 0;
	uint32_t shift = 0;
	while (data < dataEnd)
	{
		uint8_t b = *data;
		outVal |= (b & 127) << shift;
		shift += 7;
		data++;
		if (!(b & 128))
			break;
	}
	//	outVal = v;
}


static int32_t smolv_ZigDecode(uint32_t u)
{
	return (u & 1) ? ((u >> 1) ^ ~0) : (u >> 1);
}


// Remap most common Op codes (Load, Store, Decorate, VectorShuffle etc.) to be in < 16 range, for
// more compact varint encoding. This basically swaps rarely used op values that are < 16 with the
// ones that are common.

static SpvOp smolv_RemapOp(SpvOp op)
{
#	define _SMOLV_SWAP_OP(op1,op2) if (op==op1) return op2; if (op==op2) return op1
// >>>>> SPIRVCRUNCHER Block Start >>>>> SMOLSWAP_SpvOpDecorate
	_SMOLV_SWAP_OP(SpvOpDecorate, SpvOpNop); // 0: 24%
// >>>>> SPIRVCRUNCHER Block End >>>>> SMOLSWAP_SpvOpDecorate
// >>>>> SPIRVCRUNCHER Block Start >>>>> SMOLSWAP_SpvOpLoad
	_SMOLV_SWAP_OP(SpvOpLoad, SpvOpUndef); // 1: 17%
// >>>>> SPIRVCRUNCHER Block End >>>>> SMOLSWAP_SpvOpLoad
// >>>>> SPIRVCRUNCHER Block Start >>>>> SMOLSWAP_SpvOpStore
	_SMOLV_SWAP_OP(SpvOpStore, SpvOpSourceContinued); // 2: 9%
// >>>>> SPIRVCRUNCHER Block End >>>>> SMOLSWAP_SpvOpStore
// >>>>> SPIRVCRUNCHER Block Start >>>>> SMOLSWAP_SpvOpAccessChain
	_SMOLV_SWAP_OP(SpvOpAccessChain, SpvOpSource); // 3: 7.2%
// >>>>> SPIRVCRUNCHER Block End >>>>> SMOLSWAP_SpvOpAccessChain
// >>>>> SPIRVCRUNCHER Block Start >>>>> SMOLSWAP_SpvOpVectorShuffle
	_SMOLV_SWAP_OP(SpvOpVectorShuffle, SpvOpSourceExtension); // 4: 5.0%
// >>>>> SPIRVCRUNCHER Block End >>>>> SMOLSWAP_SpvOpVectorShuffle
	// Name - already small enum value - 5: 4.4%
	// MemberName - already small enum value - 6: 2.9%
// >>>>> SPIRVCRUNCHER Block Start >>>>> SMOLSWAP_SpvOpMemberDecorate
	_SMOLV_SWAP_OP(SpvOpMemberDecorate, SpvOpString); // 7: 4.0%
// >>>>> SPIRVCRUNCHER Block End >>>>> SMOLSWAP_SpvOpMemberDecorate
// >>>>> SPIRVCRUNCHER Block Start >>>>> SMOLSWAP_SpvOpLabel
	_SMOLV_SWAP_OP(SpvOpLabel, SpvOpLine); // 8: 0.9%
// >>>>> SPIRVCRUNCHER Block End >>>>> SMOLSWAP_SpvOpLabel
// >>>>> SPIRVCRUNCHER Block Start >>>>> SMOLSWAP_SpvOpVariable
	_SMOLV_SWAP_OP(SpvOpVariable, (SpvOp)9); // 9: 3.9%
// >>>>> SPIRVCRUNCHER Block End >>>>> SMOLSWAP_SpvOpVariable
// >>>>> SPIRVCRUNCHER Block Start >>>>> SMOLSWAP_SpvOpFMul
	_SMOLV_SWAP_OP(SpvOpFMul, SpvOpExtension); // 10: 3.9%
// >>>>> SPIRVCRUNCHER Block End >>>>> SMOLSWAP_SpvOpFMul
// >>>>> SPIRVCRUNCHER Block Start >>>>> SMOLSWAP_SpvOpFAdd
	_SMOLV_SWAP_OP(SpvOpFAdd, SpvOpExtInstImport); // 11: 2.5%
// >>>>> SPIRVCRUNCHER Block End >>>>> SMOLSWAP_SpvOpFAdd
	// ExtInst - already small enum value - 12: 1.2%
	// VectorShuffleCompact - already small enum value - used for compact shuffle encoding
// >>>>> SPIRVCRUNCHER Block Start >>>>> SMOLSWAP_SpvOpTypePointer
	_SMOLV_SWAP_OP(SpvOpTypePointer, SpvOpMemoryModel); // 14: 2.2%
// >>>>> SPIRVCRUNCHER Block End >>>>> SMOLSWAP_SpvOpTypePointer
// >>>>> SPIRVCRUNCHER Block Start >>>>> SMOLSWAP_SpvOpFNegate
	_SMOLV_SWAP_OP(SpvOpFNegate, SpvOpEntryPoint); // 15: 1.1%
// >>>>> SPIRVCRUNCHER Block End >>>>> SMOLSWAP_SpvOpFNegate
#	undef _SMOLV_SWAP_OP
	return op;
}


// For most compact varint encoding of common instructions, the instruction length should come out
// into 3 bits (be <8). SPIR-V instruction lengths are always at least 1, and for some other
// instructions they are guaranteed to be some other minimum length. Adjust the length before encoding,
// and after decoding accordingly.

static uint32_t smolv_DecodeLen(SpvOp op, uint32_t len)
{
	len++;
// >>>>> SPIRVCRUNCHER Block Start >>>>> DecodeLen_SpvOpVectorShuffle1
	if (op == SpvOpVectorShuffle)			len += 4;
// >>>>> SPIRVCRUNCHER Block End >>>>> DecodeLen_SpvOpVectorShuffle1
// >>>>> SPIRVCRUNCHER Block Start >>>>> DecodeLen_SpvOpVectorShuffleCompact
	if (op == SpvOpVectorShuffleCompact)	len += 4;
// >>>>> SPIRVCRUNCHER Block End >>>>> DecodeLen_SpvOpVectorShuffleCompact
// >>>>> SPIRVCRUNCHER Block Start >>>>> DecodeLen_SpvOpDecorate
	if (op == SpvOpDecorate)				len += 2;
// >>>>> SPIRVCRUNCHER Block End >>>>> DecodeLen_SpvOpDecorate
// >>>>> SPIRVCRUNCHER Block Start >>>>> DecodeLen_SpvOpLoad
	if (op == SpvOpLoad)					len += 3;
// >>>>> SPIRVCRUNCHER Block End >>>>> DecodeLen_SpvOpLoad
// >>>>> SPIRVCRUNCHER Block Start >>>>> DecodeLen_SpvOpAccessChain
	if (op == SpvOpAccessChain)				len += 3;
// >>>>> SPIRVCRUNCHER Block End >>>>> DecodeLen_SpvOpAccessChain
	return len;
}


// Shuffling bits of length + opcode to be more compact in varint encoding in typical cases:
// 0x LLLL OOOO is how SPIR-V encodes it (L=length, O=op), we shuffle into:
// 0x LLLO OOLO, so that common case (op<16, len<8) is encoded into one byte.


void smolv_ReadLengthOp(const uint8_t*& data, const uint8_t* dataEnd, uint32_t& outLen, SpvOp& outOp)
{
	uint32_t val;
	smolv_ReadVarint(data, dataEnd, val);

	outLen = ((val >> 20) << 4) | ((val >> 4) & 0xF);
	outOp = (SpvOp)(((val >> 4) & 0xFFF0) | (val & 0xF));

	outOp = smolv_RemapOp(outOp);
	outLen = smolv_DecodeLen(outOp, outLen);
}


void smolv::TinyDecode(const uint8_t* bytes, size_t smolvSize, uint8_t* outSpirv)
{
	const uint8_t* bytesEnd = bytes + smolvSize;

	uint32_t val;

	// header
	smolv_Write4(outSpirv, kSpirVHeaderMagic); bytes += 4;
	smolv_Read4(bytes, bytesEnd, val); val &= 0x00FFFFFF; smolv_Write4(outSpirv, val); // version
	smolv_Read4(bytes, bytesEnd, val); smolv_Write4(outSpirv, val); // generator
	smolv_Read4(bytes, bytesEnd, val); smolv_Write4(outSpirv, val); // bound
	smolv_Read4(bytes, bytesEnd, val); smolv_Write4(outSpirv, val); // schema
	bytes += 4; // decode buffer size

	const int knownOpsCount = SpvOpGroupNonUniformQuadSwap + 1;

	uint32_t prevResult = 0;
	uint32_t prevDecorate = 0;

	while (bytes < bytesEnd)
	{
		// read length + opcode
		uint32_t instrLen;
		SpvOp op;
		smolv_ReadLengthOp(bytes, bytesEnd, instrLen, op);
		const bool wasSwizzle = (op == SpvOpVectorShuffleCompact);
// >>>>> SPIRVCRUNCHER Block Start >>>>> wasSwizzleVectorSuffle
		if (wasSwizzle)
			op = SpvOpVectorShuffle;
// >>>>> SPIRVCRUNCHER Block End >>>>> wasSwizzleVectorSuffle
		smolv_Write4(outSpirv, (instrLen << 16) | op);

		size_t ioffs = 1;

		// read type as varint, if we have it
// >>>>> SPIRVCRUNCHER Block Start >>>>> smolv_OpHasType
		if (smolv_OpHasType(op, knownOpsCount))
		{
			smolv_ReadVarint(bytes, bytesEnd, val);
			smolv_Write4(outSpirv, val);
			ioffs++;
		}
// >>>>> SPIRVCRUNCHER Block End >>>>> smolv_OpHasType
		// read result as delta+varint, if we have it
// >>>>> SPIRVCRUNCHER Block Start >>>>> smolv_OpHasResult
		if (smolv_OpHasResult(op, knownOpsCount))
		{
			smolv_ReadVarint(bytes, bytesEnd, val);
			val = prevResult + smolv_ZigDecode(val);
			smolv_Write4(outSpirv, val);
			prevResult = val;
			ioffs++;
		}
// >>>>> SPIRVCRUNCHER Block End >>>>> smolv_OpHasResult
		// Decorate: IDs relative to previous decorate
// >>>>> SPIRVCRUNCHER Block Start >>>>> SpvDecorate
		if (op == SpvOpDecorate || op == SpvOpMemberDecorate)
		{
			smolv_ReadVarint(bytes, bytesEnd, val);
			// "before zero" version did not use zig encoding for the value
			val = prevDecorate + (smolv_ZigDecode(val));
			smolv_Write4(outSpirv, val);
			prevDecorate = val;
			ioffs++;
		}
// >>>>> SPIRVCRUNCHER Block End >>>>> SpvDecorate
// >>>>> SPIRVCRUNCHER Block Start >>>>> SpvMemberDecorate
		// MemberDecorate special decoding
		if (op == SpvOpMemberDecorate)
		{
			int count = *bytes++;
			int prevIndex = 0;
			int prevOffset = 0;
			for (int m = 0; m < count; ++m)
			{
				// read member index
				uint32_t memberIndex;
				smolv_ReadVarint(bytes, bytesEnd, memberIndex);
				memberIndex += prevIndex;
				prevIndex = memberIndex;

				// decoration (and length if not common/known)
				uint32_t memberDec;
				smolv_ReadVarint(bytes, bytesEnd, memberDec);
				const int knownExtraOps = smolv_DecorationExtraOps(memberDec);
				uint32_t memberLen;
// >>>>> SPIRVCRUNCHER BlockInBlock Start >>>>> BlockInBlock_knownExtraOpsCondition
				if (knownExtraOps == -1)
				{
					smolv_ReadVarint(bytes, bytesEnd, memberLen);
					memberLen += 4;
				}
				else
// >>>>> SPIRVCRUNCHER BlockInBlock End >>>>> BlockInBlock_knownExtraOpsCondition
					memberLen = 4 + knownExtraOps;

				// write SPIR-V op+length (unless it's first member decoration, in which case it was written before)
				if (m != 0)
				{
					smolv_Write4(outSpirv, (memberLen << 16) | op);
					smolv_Write4(outSpirv, prevDecorate);
				}
				smolv_Write4(outSpirv, memberIndex);
				smolv_Write4(outSpirv, memberDec);
// >>>>> SPIRVCRUNCHER BlockInBlock Start >>>>> BlockInBlock_OffsetDecoration
				// Special case for Offset decorations
				if (memberDec == 35) // Offset
				{
					smolv_ReadVarint(bytes, bytesEnd, val);
					val += prevOffset;
					smolv_Write4(outSpirv, val);
					prevOffset = val;
				}
				else
// >>>>> SPIRVCRUNCHER BlockInBlock End >>>>> BlockInBlock_OffsetDecoration
				{
					for (uint32_t i = 4; i < memberLen; ++i)
					{
						smolv_ReadVarint(bytes, bytesEnd, val);
						smolv_Write4(outSpirv, val);
					}
				}
			}
			continue;
		}
// >>>>> SPIRVCRUNCHER Block End >>>>> SpvMemberDecorate
		
		// Read this many IDs, that are relative to result ID
		int relativeCount = smolv_OpDeltaFromResult(op, knownOpsCount);

		for (int i = 0; i < relativeCount && ioffs < instrLen; ++i, ++ioffs)
		{
			smolv_ReadVarint(bytes, bytesEnd, val);
			val = smolv_ZigDecode(val);
			smolv_Write4(outSpirv, prevResult - val);
		}

		if (wasSwizzle && instrLen <= 9)
		{
			uint32_t swizzle = *bytes++;
// >>>>> SPIRVCRUNCHER Block Start >>>>> wasSizzleInstrLen9_5
			if (instrLen > 5) smolv_Write4(outSpirv, (swizzle >> 6) & 3);
// >>>>> SPIRVCRUNCHER Block End >>>>> wasSizzleInstrLen9_5
// >>>>> SPIRVCRUNCHER Block Start >>>>> wasSizzleInstrLen9_6
			if (instrLen > 6) smolv_Write4(outSpirv, (swizzle >> 4) & 3);
// >>>>> SPIRVCRUNCHER Block End >>>>> wasSizzleInstrLen9_6
// >>>>> SPIRVCRUNCHER Block Start >>>>> wasSizzleInstrLen9_7
			if (instrLen > 7) smolv_Write4(outSpirv, (swizzle >> 2) & 3);
// >>>>> SPIRVCRUNCHER Block End >>>>> wasSizzleInstrLen9_7
// >>>>> SPIRVCRUNCHER Block Start >>>>> wasSizzleInstrLen9_8
			if (instrLen > 8) smolv_Write4(outSpirv, swizzle & 3);
// >>>>> SPIRVCRUNCHER Block End >>>>> wasSizzleInstrLen9_8
		}
// >>>>> SPIRVCRUNCHER Block Start >>>>> OpvarRest
		else if (smolv_OpVarRest(op, knownOpsCount))
		{
			// read rest of words with variable encoding
			for (; ioffs < instrLen; ++ioffs)
			{
				smolv_ReadVarint(bytes, bytesEnd, val);
				smolv_Write4(outSpirv, val);
			}
		}
// >>>>> SPIRVCRUNCHER Block End >>>>> OpvarRest
// >>>>> SPIRVCRUNCHER Block Start >>>>> RestWithoutAnyEncoding
		else
		{
			// read rest of words without any encoding
			for (; ioffs < instrLen; ++ioffs)
			{
				smolv_Read4(bytes, bytesEnd, val);
				smolv_Write4(outSpirv, val);
			}
		}
// >>>>> SPIRVCRUNCHER Block End >>>>> RestWithoutAnyEncoding
	}
}


// ------------------------------------------------------------------------------
// This software is available under 2 licenses -- choose whichever you prefer.
// ------------------------------------------------------------------------------
// ALTERNATIVE A - MIT License
// Copyright (c) 2016-2024 Aras Pranckevicius
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ------------------------------------------------------------------------------
// ALTERNATIVE B - Public Domain (www.unlicense.org)
// This is free and unencumbered software released into the public domain.
// Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
// software, either in source code form or as a compiled binary, for any purpose,
// commercial or non-commercial, and by any means.
// In jurisdictions that recognize copyright laws, the author or authors of this
// software dedicate any and all copyright interest in the software to the public
// domain. We make this dedication for the benefit of the public at large and to
// the detriment of our heirs and successors. We intend this dedication to be an
// overt act of relinquishment in perpetuity of all present and future rights to
// this software under copyright law.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ------------------------------------------------------------------------------
