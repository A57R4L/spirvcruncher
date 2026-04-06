// spirvcruncher.cpp - spir-v processing tool
// 
// (c) 2025 Ossi Luoto

#include "smolv.h"

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <windows.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <filesystem>

#include "generated_shadertemplate.h"

using namespace std;
using namespace smolv;
namespace fs = std::filesystem;

size_t headerToSkip = 24;

struct ShaderInput {
	string filename;
	string arrayName;
};

struct EncodedShader {
	string name;
	ByteArray spirv;
	ByteArray smolv;
	size_t decodedSize;
};

static bool loadBinaryFile(const string& inFilePath, vector<uint8_t>& output)
{
	ifstream input(inFilePath, ios::binary);
	if (!input) return false;

	// Clear output buffer
	output.clear();

	// Seek to end to get size
	input.seekg(0, ios::end);
	streamsize size = input.tellg();
	input.seekg(0, ios::beg);

	if (size > 0) {
		output.resize(size);
		if (!input.read(reinterpret_cast<char*>(output.data()), size)) {
			return false;
		}
	}

	return true;
}

static bool saveBinaryToArray(
	const vector<uint8_t>& data,
	const string& headerFilePath,
	const string& arrayName,
	size_t decodedsize)
{
	ofstream output(headerFilePath);
	if (!output) return false;

	output << "// Generated with spirvcruncher\n\n";
	output << "#pragma once\n";
	output << "const uint8_t " << arrayName << "[] = {\n\n";

	size_t count = 0;
	for (size_t i = 0; i < data.size(); ++i) {
		if (count % 12 == 0) {
			output << "    ";
		}

		output << "0x" << std::hex << std::setw(2) << std::setfill('0')
			<< static_cast<int>(data[i]);

		if (i != data.size() - 1) {
			output << ", ";
		}

		++count;

		if (count % 12 == 0) {
			output << "\n";
		}
	}

	output << "\n};\n\n";
	output << std::dec << std::setw(0) << std::setfill(' ');
	output << "const size_t " << arrayName << "_encoded_sizeInBytes = " << data.size() << ";\n";
	output << "const size_t " << arrayName << "_sizeInBytes = " << decodedsize << "; \n";

	return true;
}

string getExecutableFolder() {

	char buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);

	string fullPath(buffer);
	size_t pos = fullPath.find_last_of("\\/");

	return fullPath.substr(0, pos);
}

static bool checkEntryFromBlocks(DecodeAnalysis& analysis, string entryCheck)
{
	bool bResult = false;
	for (auto block : analysis.Blocks)
	{
		// Check for block entry within the line
		if (entryCheck.find(block.entry) != string::npos)
		{
			bResult = true;
			break;
		}
	}
	return bResult;
}

static bool checkEntryFromSpv(DecodeAnalysis& analysis, string entryCheck)
{
	bool bResult = false;
	for (auto op : analysis.SpvOps)
	{
		// Check for block entry within the line
		if (entryCheck.find(op.entry) != string::npos)
		{
			bResult = true;
			break;
		}
	}
	return bResult;
}

static bool copyTemplateWithConditions(istringstream& templateFile, ofstream& outputFile, DecodeAnalysis& analysis, bool bSkipOptimizer)
{
	string line;
	int lineNumber = 0;
	int spvLineNumber = 0;
	bool bBlockSegment = false;
	bool bSpvSegment = false;
	bool bBlockModeOn = false;

	// For at least special case for offset decorations
	bool bBlockInBlock = false;
	bool bBlockInBlockModeOn = false;

	// For removing segments altogether
	bool bRemoveSegment = false;

	// Main loop, look for lines starting with our trigger code, copy/replace with conditions

	while (getline(templateFile, line)) {
		lineNumber++;

		// Start of block optimization
		if (!bSpvSegment && line.find("SPIRVCRUNCHER Block Start") != string::npos)
		{
			bBlockSegment = true;

			// Check if we have this segment in our database
			bBlockModeOn = checkEntryFromBlocks(analysis, line) || bSkipOptimizer;
			continue;  // Skip the declaration lines
		}

		// Start of Spv chunk
		if (!bBlockSegment && line.find("SPIRVCRUNCHER Spv Start") != string::npos)
		{
			bSpvSegment = true;
			continue;  // Skip the declaration line
		}

		// End of Spv chunk
		if (bSpvSegment && line.find("SPIRVCRUNCHER Spv End") != string::npos)
		{
			bSpvSegment = false;
			continue;  // Skip the declaration line
		}

		// End of Block chunk
		if (bBlockSegment && line.find("SPIRVCRUNCHER Block End") != string::npos)
		{
			bBlockSegment = false;
			bBlockModeOn = false;
			continue;
		}

		// 
		// Remove completely on build
		// 
		
		// Start of Remove segment
		if (!bRemoveSegment && line.find("SPIRVCRUNCHER Remove on build start") != string::npos)
		{
			bRemoveSegment = true;
			continue;  // Skip the declaration line
		}

		// End of Remove segment
		if (bRemoveSegment && line.find("SPIRVCRUNCHER Remove on build end") != string::npos)
		{
			bRemoveSegment = false;
			continue;  // Skip the declaration line
		}

		// Skip if remove segment mode one
		if (bRemoveSegment) continue;

		// Skip if deleteline
		if (line.find("SPIRVCRUNCHER skip on build") != string::npos) continue;

		// In blockmode, we copy only lines that are included in our database
		if (bBlockSegment && bBlockModeOn)
		{
			// Likely in copy mode, but check first special conditions
			if (!bBlockInBlock && line.find("SPIRVCRUNCHER BlockInBlock Start") != string::npos)
			{
				bBlockInBlock = true;
				bBlockInBlockModeOn = checkEntryFromBlocks(analysis, line) || bSkipOptimizer;
				continue;
			}

			if (bBlockInBlock && line.find("SPIRVCRUNCHER BlockInBlock End") != string::npos)
			{
				bBlockInBlock = false;
				bBlockInBlockModeOn = false;
				continue;
			}

			// Skip write if we are in block in block, but don't have blockinblock write-mode on
			if (bBlockInBlock && !bBlockInBlockModeOn) continue;

			// Else write
			outputFile << line << '\n';
			continue;
		}

		// In Spvmode, check if have the op in question in our database, else fill with empty
		if (bSpvSegment)
		{
			// Check analysis, or copy also if we are skipping optimizer altogether
			if (checkEntryFromSpv(analysis, to_string(spvLineNumber)) || bSkipOptimizer)
			{
				outputFile << line << '\n';
			}
			else
			{
				// This is our best attempt to give crinkler size optimization opportunities for op-data
				outputFile << "		{0, 0, 0, 0}, // SPIRVCRUNCHER - op " << spvLineNumber << "not in use\n";
			}
			spvLineNumber++;
			continue;
		}

		// Else copy if we are not block or spv mode
		if (!bSpvSegment && !bBlockSegment)
		{
			outputFile << line << '\n';
			continue;
		}
	}

	if (bSpvSegment || bBlockSegment) return false;

	// Implement other fail checks?
	return true;
}

static bool generateUberHeader(
	istringstream& templateFile,
	ofstream& outputFile,
	DecodeAnalysis& analysis,
	const vector<EncodedShader>& shaders,
	bool bSkipOptimizer, bool bSkipCruncher)
{
	bool bResult = true;

	//
	// 1. Header 
	//

	{
		outputFile << "//\n// Generated with spirvcruncher on: ";
		// Timestamp
		auto now = chrono::system_clock::now();
		time_t now_time = chrono::system_clock::to_time_t(now);
		tm* local_time = localtime(&now_time);
		outputFile << std::put_time(local_time, "%Y-%m-%d %H:%M:%S");

		outputFile << "\n//\n";

		string line;
		bool bSegmentDone = false;
		while (!bSegmentDone) {
			getline(templateFile, line);

			if (line.find("SPIRVCRUNCHER Shaderblock") != string::npos)
			{
				bSegmentDone = true;
				continue;
			}
			else
			{
				outputFile << line << '\n';
			}
		}
	}

	//
	// 2. Shadercode
	//
	// First, check if all shaders share the same SPIR-V Version to optimize size

	bool allVersionsMatch = true;
	uint32_t sharedVersionWord = 0;

	for (size_t i = 0; i < shaders.size(); ++i) {
		// Reconstruct the 32-bit version word correctly from little-endian bytes
		uint32_t versionWord = shaders[i].spirv[4] |
			(shaders[i].spirv[5] << 8) |
			(shaders[i].spirv[6] << 16) |
			(shaders[i].spirv[7] << 24);

		if (i == 0) sharedVersionWord = versionWord;
		else if (versionWord != sharedVersionWord) allVersionsMatch = false;
	}

	if (allVersionsMatch && !shaders.empty()) {
		outputFile << "constexpr uint32_t shared_spvVersion = 0x"
			<< std::hex << std::setw(8) << std::setfill('0') << sharedVersionWord << std::dec << ";\n\n";
	}

	// PASS 1: Group all packed bytes together in one Data Segment

	outputFile << "// --- Compressed Shader Payloads ---\n";
	outputFile << "#pragma data_seg(\".smolv\")\n\n";

	for (const auto& shader : shaders) {
		// For debugging
		size_t skipHeader = bSkipCruncher ? 0 : headerToSkip;
		size_t dataSizeNoHeader = shader.smolv.size() - skipHeader;

		outputFile << "const uint8_t " << shader.name << "[] = {\n\n";

		size_t count = 0;
		for (size_t i = 0; i < dataSizeNoHeader; ++i) {
			if (count % 12 == 0) outputFile << "    ";

			outputFile << "0x" << std::hex << std::setw(2) << std::setfill('0')
				<< static_cast<int>(shader.smolv[i + skipHeader]);

			if (i != dataSizeNoHeader - 1) outputFile << ", ";
			++count;
			if (count % 12 == 0) outputFile << "\n";
		}

		outputFile << "\n};\n\n";
	}

	// Reset data segment to default
	outputFile << "#pragma data_seg()\n\n";

	// PASS 2: Group all metadata together

	outputFile << "// --- Metadata ---\n";
	for (const auto& shader : shaders) {
		size_t skipHeader = bSkipCruncher ? 0 : headerToSkip;
		size_t dataSizeNoHeader = shader.smolv.size() - skipHeader;

		outputFile << std::dec << std::setw(0) << std::setfill(' ');
		outputFile << "constexpr size_t " << shader.name << "_encoded_sizeInBytes = " << dataSizeNoHeader << ";\n";
		outputFile << "constexpr size_t " << shader.name << "_sizeInBytes = " << shader.decodedSize << ";\n";

		if (!bSkipCruncher) {
			if (!allVersionsMatch) {
				uint32_t versionWord = shader.spirv[4] | (shader.spirv[5] << 8) | (shader.spirv[6] << 16) | (shader.spirv[7] << 24);
				outputFile << "constexpr uint32_t " << shader.name << "_spvVersion = 0x"
					<< std::hex << std::setw(8) << std::setfill('0') << versionWord << std::dec << ";\n";
			}

			uint32_t boundWord = shader.spirv[12] | (shader.spirv[13] << 8) | (shader.spirv[14] << 16) | (shader.spirv[15] << 24);
			outputFile << "constexpr uint32_t " << shader.name << "_spvBound = 0x"
				<< std::hex << std::setw(8) << std::setfill('0') << boundWord << std::dec << ";\n";
		}
		outputFile << "\n";
	}



	/*
		outputFile << std::dec << std::setw(0) << std::setfill(' ');
//		outputFile << "constexpr size_t " << shader.name << "_encoded_sizeInBytes = " << dataSizeNoHeader << ";\n";
		outputFile << "constexpr size_t " << shader.name << "_sizeInBytes = " << shader.decodedSize << "; \n";

		if (!bSkipCruncher)
		{
			if (!allVersionsMatch) {
				uint32_t versionWord = shader.spirv[4] | (shader.spirv[5] << 8) | (shader.spirv[6] << 16) | (shader.spirv[7] << 24);
				outputFile << "constexpr uint32_t " << shader.name << "_spvVersion = 0x"
					<< std::hex << std::setw(8) << std::setfill('0') << versionWord << std::dec << ";\n";
			}

			// Reconstruct Bound correctly
			uint32_t boundWord = shader.spirv[12] | (shader.spirv[13] << 8) | (shader.spirv[14] << 16) | (shader.spirv[15] << 24);
			outputFile << "constexpr uint32_t " << shader.name << "_spvBound = 0x"
				<< std::hex << std::setw(8) << std::setfill('0') << boundWord << std::dec << ";\n";
		}
		*/

	// PASS 3: Group all uninitialized buffers in the BSS Segment

	outputFile << "// --- Uninitialized Memory Buffers (BSS) ---\n";
	outputFile << "#pragma bss_seg(\".spirvbss\")\n\n";

	for (const auto& shader : shaders) {
		size_t bufferWords = (shader.decodedSize + 3) / 4;

		outputFile << "inline uint32_t " << shader.name << "_buffer[" << bufferWords << "];\n";
	}

	// Reset bss segment to default
	outputFile << "\n#pragma bss_seg()\n\n";

	/*
		// Allocate zero-initialized 32-bit aligned space
		size_t bufferWords = (shader.decodedSize + 3) / 4;
		outputFile << "inline uint32_t " << shader.name << "_buffer[" << bufferWords << "] = {};\n\n";
	}
	*/

	// Generate debug "decoder" and macro
	if (bSkipCruncher)
	{
		outputFile << "// BYPASS MODE: smol-v decrunch skipped. Doing raw 32-bit copy.\n";
		outputFile << "inline void decrunch_bypass(const uint8_t* src, size_t sizeInBytes, uint32_t* dst) {\n";
		outputFile << "\tconst uint32_t* src32 = (const uint32_t*)src;\n";
		outputFile << "\tfor (size_t i = 0; i < sizeInBytes / 4; ++i) {\n";
		outputFile << "\t\tdst[i] = src32[i];\n";
		outputFile << "\t}\n";
		outputFile << "}\n\n";

		outputFile << "#define DECRUNCH_ALL_SHADERS() \\\n";
		for (size_t i = 0; i < shaders.size(); ++i) {
			const auto& s = shaders[i];
			outputFile << "\tdecrunch_bypass(" << s.name << ", " << s.name << "_sizeInBytes, " << s.name << "_buffer)";
			if (i < shaders.size() - 1) outputFile << "; \\\n";
			else outputFile << "\n\n";
		}
	}
	else
	{
		outputFile << "// Macro to decrunch all shaders into their respective buffers\n";
		outputFile << "#define DECRUNCH_ALL_SHADERS() \\\n";
		for (size_t i = 0; i < shaders.size(); ++i) {
			const auto& s = shaders[i];
			string v = allVersionsMatch ? "shared_spvVersion" : s.name + "_spvVersion";
			outputFile << "\tdecrunch(" << s.name << ", " << s.name << " + " << s.name << "_encoded_sizeInBytes, " << v << ", " << s.name << "_spvBound, (uint8_t*)" << s.name << "_buffer)";
			if (i < shaders.size() - 1) outputFile << "; \\\n";
			else outputFile << "\n\n";
		}

		bResult = copyTemplateWithConditions(templateFile, outputFile, analysis, bSkipOptimizer);
		if (!bResult) return false;
	}

	return true;
}


int main(int argc, char* argv[])
{
	vector<ShaderInput> inputs;
	string filenameOut = "spirvcrunchedshaders.h";
	bool bStripEncodeFlags = false;
	bool bSilent = false;
	bool bSkipOptimizer = false;   // For sanity checking that the code optimizer is working as intended
	bool bSkipCruncher = false;    // For sanity checking that smol-v packer is working, this means in practice that decrunch is just a copy operation

	string currentFile = "";
	string currentName = "";

	// Parse input arguments to allow multiple -i / -n pairs
	for (int i = 1; i < argc; ++i) {
		string arg = argv[i];

		if (arg == "-i" || arg == "--input") {
			if (i + 1 < argc) {
				string val = argv[++i];

				// Handle wildcard inputs like "*.spv" or "data/*.spv"
				if (val.find('*') != string::npos) {
					fs::path p(val);
					fs::path dir = p.parent_path();
					if (dir.empty()) dir = "."; // Current directory
					string ext = p.extension().string();

					if (fs::exists(dir) && fs::is_directory(dir)) {
						for (const auto& entry : fs::directory_iterator(dir)) {
							if (entry.path().extension() == ext) {
								inputs.push_back({ entry.path().string(), entry.path().stem().string() });
							}
						}
					}
				}
				else {
					// Handle normal single file input
					if (!currentFile.empty()) {
						inputs.push_back({ currentFile, currentName.empty() ? fs::path(currentFile).stem().string() : currentName });
						currentName = "";
					}
					currentFile = val;
				}
			}
		}
		else if (arg == "-n" || arg == "--name") {
			if (i + 1 < argc) currentName = argv[++i];
		}
		else if (arg == "-o" || arg == "--output") {
			if (i + 1 < argc) filenameOut = argv[++i];
		}
		else if (arg == "-d" || arg == "--stripdebuginfo") {
			bStripEncodeFlags = true;
		}
		else if (arg == "-s" || arg == "--silent") {
			bSilent = true;
		}
		else if (arg == "--skipoptimizer") {
			bSkipOptimizer = true;
		}
		else if (arg == "--skipcruncher") {
			bSkipCruncher = true;
			bSkipOptimizer = true;
		}
		else {
			cerr << "Unknown option: " << arg << endl;
			return 1;
		}
	}

	// Push the final parsed input if it was a single file
	if (!currentFile.empty()) {
		inputs.push_back({ currentFile, currentName.empty() ? fs::path(currentFile).stem().string() : currentName });
	}

	if (inputs.empty())
	{
		cerr << "Usage: " << argv[0] << " -i <shader1.spv> [-n <name1>] [-i <shader2.spv> [-n <name2>]] [-o <output_header>] [-d] [-s]\n";
		return 1;
	}

	vector<EncodedShader> processedShaders;
	DecodeAnalysis globalAnalysis;
	bool bResult = true;

	for (const auto& input : inputs) {
		if (!bSilent) cout << "Processing: " << input.filename << " as " << input.arrayName << endl;

		ByteArray spirv, smolv;
		if (!loadBinaryFile(input.filename, spirv) || spirv.empty()) {
			cerr << "Failed to read: " << input.filename << endl;
			return 1;
		}

		size_t decodedSize = 0;

		if (bSkipCruncher)
		{
			// just copy
			smolv = spirv;
			decodedSize = spirv.size();
		}
		else
		{
			// Encode to smol-v
			if (!Encode(spirv.data(), spirv.size(), smolv, bStripEncodeFlags ? kEncodeFlagStripDebugInfo : 0)) {
				cerr << "Failed to encode smolv: " << input.filename << endl;
				return 1;
			}

			decodedSize = GetDecodedBufferSize(smolv.data(), smolv.size());
			if (decodedSize > 0) {
				ByteArray returnspirv;
				returnspirv.resize(decodedSize);
				DecodeAnalysis localAnalysis;

				if (DecodeWithAnalysis(smolv.data(), smolv.size(), returnspirv.data(), decodedSize, &localAnalysis, kDecodeFlagNone)) {
					// Merge local blocks into global
					for (const auto& block : localAnalysis.Blocks) {
						bool found = false;
						for (auto& gBlock : globalAnalysis.Blocks) {
							if (gBlock.entry == block.entry) { gBlock.count += block.count; found = true; break; }
						}
						if (!found) globalAnalysis.Blocks.push_back(block);
					}

					// Merge local ops into global
					for (const auto& op : localAnalysis.SpvOps) {
						bool found = false;
						for (auto& gOp : globalAnalysis.SpvOps) {
							if (gOp.entry == op.entry) { gOp.count += op.count; found = true; break; }
						}
						if (!found) globalAnalysis.SpvOps.push_back(op);
					}
				}
			}
		}
		processedShaders.push_back({ input.arrayName, spirv, smolv, decodedSize });
	}

	// Output logic
	if (bResult)
	{
		istringstream templateFile(shadertemplate); // From generated_shadertemplate.h
		ofstream outFile(filenameOut);

		if (!templateFile || !outFile) {
			cerr << "Cannot open template or output files" << std::endl;
			return 1;
		}

		bResult = generateUberHeader(templateFile, outFile, globalAnalysis, processedShaders, bSkipOptimizer, bSkipCruncher);
		if (!bResult) {
			cerr << "Error creating .h file" << std::endl;
			return 1;
		}

		if (!bSilent) cout << "Successfully created combined header: " << filenameOut << " with " << processedShaders.size() << " shaders." << std::endl;
	}

	return bResult ? 0 : 1;
}
