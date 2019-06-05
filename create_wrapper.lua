-- Copyright (C) 2019 Miku AuahDark
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the "Software"), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
-- copies of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in all
-- copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
-- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
-- SOFTWARE.

local arg = {...}
local glh = assert(io.open(assert(arg[1], "please specify GL/gl.h file"), "rb"))
local gdih = assert(io.open(assert(arg[2], "please specify wingdi.h file"), "rb"))
local out = assert(io.open(assert(arg[3], "please output file"), "wb"))
local def = assert(io.open(assert(arg[4], "please def file"), "wb"))

local funcs = {}

--gl.h
for line in glh:lines() do
	line = line:gsub("[\r|\n]", "")
	local retval, name, paramstr = line:match("^WINGDIAPI (.+)APIENTRY (%w+)%s*%((.+)%);$")
	if retval then
		local params = {}
		if paramstr:sub(1, 4) ~= "void" then
			for w in paramstr:gmatch("[^,]+") do
				params[#params + 1] = w:match("(%w+)$")
			end
		end
		out:write("decltype(::", name, ") *", name, "_t = nullptr;\n")
		out:write("#ifndef WRAPGL_DISABLE_", name, "\n")
		out:write("__declspec(dllexport) ", retval, " __stdcall ", name, "(", paramstr, ")\n")
		out:write("{\n")
		out:write("\treturn ", name, "_t(", table.concat(params, ", "), ");\n")
		out:write("}\n")
		out:write("#endif\n\n")
		funcs[#funcs + 1] = name
	end
end

-- wingdi.h
do
	local iter = gdih:lines()
	for line in iter do
		local eof = false
		line = line:gsub("[\r|\n]", "")

		if line:find("^%s*WINGDIAPI .+%s*WINAPI wgl%w+") then
			-- Some are spreaded into multiple lines
			while line:find(";") == nil do
				local nextline = iter()
				if not(nextline) then
					eof = true
					break
				end

				line = line..nextline:gsub("%s%s+", ""):gsub("[\r|\n]", "")
			end

			print("found wgl: "..line)
		end

		if eof then break end

		local retval, name, paramstr = line:match("^%s*WINGDIAPI (.+)%s*WINAPI (wgl%w+)%s*%((.+)%);$")
		if retval then
			local params = {}

			if paramstr:sub(1, 4):lower() ~= "void" then
				-- wingdi doesn't have any typename
				local paramtype = {}
				for w in paramstr:gmatch("[^,]+") do
					paramtype[#paramtype + 1] = w
				end

				local newparam = {}
				for i = 1, #paramtype do
					local paramsubst = string.char(i + 96)
					newparam[i] = paramtype[i].." "..paramsubst
					params[i] = paramsubst
				end

				paramstr = table.concat(newparam, ", ")
				print("reconstructing parameters for "..name.."("..paramstr..")")
			end
			out:write("decltype(::", name, ") *", name, "_t = nullptr;\n")
			out:write("#ifndef WRAPGL_DISABLE_", name, "\n")
			out:write("__declspec(dllexport) ", retval, " __stdcall ", name, "(", paramstr, ")\n")
			out:write("{\n")
			out:write("\treturn ", name, "_t(", table.concat(params, ", "), ");\n")
			out:write("}\n")
			out:write("#endif\n\n")
			funcs[#funcs + 1] = name
		end
	end
end

def:write("EXPORTS\n")

out:write("bool initialize(HMODULE opengl32)\n")
out:write("{\n")

for _, v in ipairs(funcs) do
	out:write("\t", v, "_t = (decltype(::", v, ") *) GetProcAddress(opengl32, \"", v, "\");\n")
	out:write("\tif (", v, "_t == nullptr)\n")
	out:write("\t{\n")
	out:write("\t\tfprintf(stderr, \"Unable to find ", v, "\");\n")
	out:write("\t\treturn false;\n")
	out:write("\t}\n\n");
	def:write("    ", v, "\n")
end
out:write("\treturn true;\n}\n")

def:write(
	"    wglChoosePixelFormat\n",
	"    wglDescribePixelFormat\n",
	"    wglGetPixelFormat\n",
	"    wglSetPixelFormat\n",
	"    wglSwapBuffers\n"
)

glh:close()
gdih:close()
out:close()
def:close()
