require "kami"

function hex_dump(buf)
	for byte=1, #buf, 16 do
		local chunk = buf:sub(byte, byte+15)
		kami.cl(string.format('%08X  ',byte-1))
		chunk:gsub('.', function (c) kami.cl(string.format('%02X ',string.byte(c))) end)
		kami.cl(string.rep(' ',3*(16-#chunk)))
		kami.cl(' '..chunk:gsub('%c','.')..'\n') 
	end
end

-- Hello ^D^
kami.cl(' Lua Console\n'
		..'Type "start" to begin game, or "exit" to end.\n'
		..'You may also setup parameters now.\n'
)

-- Just incase someone trys to write to stdin...
io.write('\n--NOT AN INTERACTIVE TERMINAL--\n')

-- TODO: hook game and physics settings so they can be controlled via scripts