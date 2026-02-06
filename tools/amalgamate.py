#!/usr/bin/env python3
import os
import sys

def amalgamate(output_file="dist/beast_json.hpp"):
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    include_dir = os.path.join(base_dir, "include/beast_json")
    external_dir = os.path.join(base_dir, "external")
    
    if not os.path.exists("dist"):
        os.makedirs("dist")

    main_header = os.path.join(include_dir, "beast_json.hpp")
    
    with open(main_header, "r") as f:
        lines = f.readlines()

    with open(output_file, "w") as out:
        out.write("// Auto-generated single header for Beast JSON\n")
        out.write("// Bundles sse2neon.h if available\n\n")
        
        for line in lines:
            if '#include "sse2neon.h"' in line:
                sse_path = os.path.join(external_dir, "sse2neon.h")
                if os.path.exists(sse_path):
                    with open(sse_path, "r") as sse:
                        out.write("// BEGIN sse2neon.h\n")
                        out.write(sse.read())
                        out.write("// END sse2neon.h\n")
                else:
                    out.write("// sse2neon.h not found during amalgamation, keeping include\n")
                    out.write(line)
            else:
                out.write(line)
    
    print(f"Created {output_file}")

if __name__ == "__main__":
    amalgamate()
