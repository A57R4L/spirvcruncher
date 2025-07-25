# embed shadercode to main build
message(STATUS "INPUT=${INPUT}")
file(READ "${INPUT}" FILE_CONTENTS)

# Escape backslashes and quotes for C++ string literal
string(REPLACE "\\" "\\\\" FILE_CONTENTS "${FILE_CONTENTS}")
string(REPLACE "\"" "\\\"" FILE_CONTENTS "${FILE_CONTENTS}")
string(REPLACE "\n" "\\n\"\n\"" FILE_CONTENTS "${FILE_CONTENTS}")

file(WRITE "${OUTPUT}" "constexpr char shadertemplate[] = \"${FILE_CONTENTS}\";\n")