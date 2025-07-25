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

#include "generated_shadertemplate.h"

using namespace std;
using namespace smolv;

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

static bool copyTemplateWithConditions(istringstream& templateFile, ofstream& outputFile, DecodeAnalysis& analysis, const string& arrayName, const vector<uint8_t>& data)
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
			bBlockModeOn = checkEntryFromBlocks(analysis, line);
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

		// Special case: decruncher start
		if (line.find("SPIRVCRUNCHER Decrunch Segment") != string::npos)
		{
			outputFile << "	const uint8_t* bytes = " << arrayName << ";\n";
			outputFile << "	const uint8_t* bytesEnd = bytes + " << arrayName << "_encoded_sizeInBytes;\n";

			outputFile << "	// Header\n";
			outputFile << "	*(uint32_t*)spirvCode = 0x07230203; // Magic number (mandatory)\n";
			outputFile << "	spirvCode += 4;\n";
			outputFile << "	*(uint32_t*)spirvCode = 0x00" <<
				std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[7]) <<
				std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[6]) <<
				std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[5]) <<
				std::dec << std::setw(0) << std::setfill(' ') << "; // Version (mandatory)\n";
			outputFile << "	spirvCode += 8; // skip Generator (not mandatory)\n";
			outputFile << "	*(uint32_t*)spirvCode = 0x" <<

				std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[15]) <<
				std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[14]) <<
				std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[13]) <<
				std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[12]) <<
				std::dec << std::setw(0) << std::setfill(' ') << "; // Bound (mandatory)\n";
			outputFile << "	spirvCode += 8; // skip Schema (not used?)\n";
			outputFile << std::dec << std::setw(0);

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
				bBlockInBlockModeOn = checkEntryFromBlocks(analysis, line);
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
			if (checkEntryFromSpv(analysis, to_string(spvLineNumber)))
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
	const string& arrayName,
	const vector<uint8_t>& data,
	size_t decodedsize)
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
	{

		// Skip shader binary header - we hardcode the mandatory few bytes later

		size_t headerToSkip = 24;
		size_t dataSizeNoHeader = data.size() - headerToSkip;

		outputFile << "#pragma data_seg.(\"." << arrayName << "\")\n";
		outputFile << "const uint8_t " << arrayName << "[] = {\n\n";

		size_t count = 0;
		for (size_t i = 0; i < dataSizeNoHeader; ++i) {
			if (count % 12 == 0) {
				outputFile << "    ";
			}

			outputFile << "0x" << std::hex << std::setw(2) << std::setfill('0')
				<< static_cast<int>(data[i + headerToSkip]);

			if (i != dataSizeNoHeader - 1) {
				outputFile << ", ";
			}

			++count;

			if (count % 12 == 0) {
				outputFile << "\n";
			}
		}

		outputFile << "\n};\n\n";
		outputFile << std::dec;
		outputFile << "const size_t " << arrayName << "_encoded_sizeInBytes = " << dataSizeNoHeader << ";\n";
		outputFile << "const size_t " << arrayName << "_sizeInBytes = " << decodedsize << "; \n\n";

	}

	//
	// 3. Decode part
	//

	{
		bResult = copyTemplateWithConditions(templateFile, outputFile, analysis, arrayName, data);
	}

	return true;
}


int main(int argc, char* argv[])
{
	// Default variables for shader

	string filenameIn;
	string filenameOut = "spirvcrunchedshader.h";
	string arrayName = "spirvcrunchedshader";
	bool bStripEncodeFlags = false;

	// Default variables for .h/.cpp

	string fileOut_h = "smolv.h";
	string fileOut_cpp = "smolv.cpp";

	bool bSilent = false;

	// Parse input arguments

	for (int i = 1; i < argc; ++i) {
		string arg = argv[i];

		if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
			filenameIn = argv[++i];
		}
		else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
			filenameOut = argv[++i];
		}
		else if ((arg == "-n" || arg == "--name") && i + 1 < argc) {
			arrayName = argv[++i];
		}
		else if ((arg == "-d" || arg == "--stripdebuginfo")) {
			bStripEncodeFlags = true;
		}
		else if ((arg == "-s" || arg == "--silent")) {
			bSilent = true;
		}
		else if ((arg == "-h" || arg == "--output_h") && i + 1 < argc) {
			fileOut_h = argv[++i];
		}
		else if ((arg == "-c" || arg == "--output_cpp") && i + 1 < argc) {
			fileOut_cpp = argv[++i];
		}
		else {
			cerr << "Unknown option or missing value: " << arg << endl;
			return 1;
		}
	}

	if (filenameIn.empty())
	{
		cerr << "Use " << argv[0] << " -i <input_shaderfile> [-o <output_headerfile>] [-n <arrayname>] [-d strip debug info] [-s silent]";
		//<<
		//	"[-u (single header with decoder, default)\n" <<
		//	"[-m (multifile, shader.h, smolv.h, smol.cpp) [-h <output_decodeheaderfile>][-c <output_decodecppfile>]\n";
		return 1;
	}
	
	if (!bSilent) cout << "Running spirvcruncher for: " << filenameIn << endl;

	ByteArray spirv;
	ByteArray smolv;
	
	bool bResult = true;

	bResult = loadBinaryFile(filenameIn, spirv);

	if (!bResult || spirv.empty())
	{
		cerr << "Failed to read: " << filenameIn << endl;
		return 1;
	}

	if (!bSilent) cout << "Loading done" << endl;

	// Encode to SMOL-V, optionally strip debug info

	if (bResult)
	{

		if (!Encode(spirv.data(), spirv.size(), smolv, bStripEncodeFlags ? kEncodeFlagStripDebugInfo : 0))
		{
			cerr << "Failed to encode smolv: " << filenameIn << endl;
			bResult = false;
			return 1;
		}
	}

	if (!bSilent) cout << "Encoding done" << endl;

	// Create header file from encoded shader data
	/*
	if (bResult)
	{
		size_t decodedSize = GetDecodedBufferSize(smolv.data(), smolv.size());
		cout << "Compressed to size: " << smolv.size() << " Expected to decode to: " << decodedSize << " Original size: " << spirv.size() << endl;
	
		if (!saveBinaryToArray(smolv, filenameOut, arrayName, decodedSize))
		{
			bResult = false;
			cerr << "Failed to create output: " << filenameOut << endl;
			return 1;
		}
	}
	

	if (bResult)
	{
		cout << " Shader include file: " << filenameOut << " created" << endl;
	}
	*/
	// Run analyzer

	DecodeAnalysis analysis;

	if (bResult)
	{
		ByteArray returnspirv;
		size_t decodedSize = GetDecodedBufferSize(smolv.data(), smolv.size());
		if (decodedSize > 0)
		{
			returnspirv.resize(decodedSize);

			if (DecodeWithAnalysis(smolv.data(), smolv.size(), returnspirv.data(), decodedSize, &analysis, kDecodeFlagNone))
			{
				for (auto block : analysis.Blocks)
				{
					if (!bSilent) cout << "Block: " << block.entry << " Amount: " << block.count << endl;
				}

				for (auto op : analysis.SpvOps)
				{
					if (!bSilent) cout << "Op: " << op.entry << " Amount: " << op.count << endl;
				}
			}
		}

	}

	/*
	// Check .h/.cpp templates

	if (bResult)
	{
		string exePath = getExecutableFolder();
		ifstream template_h(exePath + "\\smolv_template.h");
		ifstream template_cpp(exePath + "\\smolv_template.cpp");

		ofstream outfile_h(fileOut_h);
		ofstream outfile_cpp(fileOut_cpp);

		if (!template_h || !template_cpp || !outfile_h || !outfile_cpp)
		{
			cerr << "Cannot open .h/.cpp files" << std::endl;
			return 1;
		}

		bResult = copyTemplateWithConditions(template_h, outfile_h, analysis);
		if (!bResult)
		{
			cerr << "Error creating .h file" << std::endl;
			return 1;
		}

		template_h.close();
		outfile_h.close();

		bResult = copyTemplateWithConditions(template_cpp, outfile_cpp, analysis);
		if (!bResult)
		{
			cerr << "Error creating .cpp file" << std::endl;
			return 1;
		}

		template_cpp.close();
		outfile_cpp.close();

	}
	*/

	if (bResult)
	{
		string exePath = getExecutableFolder();
		
		//ifstream templateFile(exePath + "\\spirvcruncher_template.h");
		// From the generated header
		istringstream templateFile(shadertemplate);

		ofstream outFile(filenameOut);

		if (!templateFile || !outFile)
		{
			cerr << "Cannot open template or output files" << std::endl;
			bResult = false;
			return 1;
		}

		size_t decodedSize = GetDecodedBufferSize(smolv.data(), smolv.size());

		bResult = generateUberHeader(templateFile, outFile, analysis, arrayName, smolv, decodedSize);
		if (!bResult)
		{
			cerr << "Error creating .h file" << std::endl;
			bResult = false;
			return 1;
		}

		//templateFile.close();
		outFile.close();

		if (!bSilent) cout << "Finished spirvcrunching shader: " << filenameOut << std::endl;
	}
	/*
	if (bResult)
	{
		if (!bSilent) cout << "Finished spirvcrunching succesfully!" << std::endl;
	}
	*/
	return 0;
}
