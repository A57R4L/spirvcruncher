// spirvcruncher.cpp - spir-v processing tool
// 
// (c) 2025 Ossi Luoto

#include "smolv.h"

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>

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

static bool saveBinaryToArray(const vector<uint8_t>& data,
	const string& headerFilePath,
	const string& arrayName,
	size_t decodedsize)
{
	ofstream output(headerFilePath);
	if (!output) return false;

	output << "// Generated with spirvcruncher\n\n";
	output << "#pragma once\n\n";
	output << "const uint8_t " << arrayName << "[] = {\n";

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
	output << std::dec;
	output << "const size_t " << arrayName << "_encoded_sizeInBytes = " << data.size() << ";\n";
	output << "const size_t " << arrayName << "_sizeInBytes = " << decodedsize << "; \n";

	return true;
}

int main(int argc, char* argv[])
{
	// Default variables

	string filenameIn;
	string filenameOut = "crunchedshader.h";
	string arrayName = "smolvshader";
	bool bStripEncodeFlags = false;

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
		else {
			cerr << "Unknown option or missing value: " << arg << endl;
			return 1;
		}
	}

	if (filenameIn.empty())
	{
		cerr << "Use " << argv[0] << " -i <input_shaderfile> [-o <output_headerfile>] [-n <arrayname>] [-d strip debug info] \n";
		return 1;
	}
	
	cout << "Running spirvcruncher for: " << filenameIn << endl;

	ByteArray spirv;
	ByteArray smolv;
	
	bool bResult = true;

	bResult = loadBinaryFile(filenameIn, spirv);

	if (!bResult || spirv.empty())
	{
		cerr << "Failed to read: " << filenameIn << endl;
		return 1;
	}

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

	// Create header file from encoded shader data

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
		cout << filenameOut << " include file created" << endl;
	}

	return 0;
}
