local function insert_file_template(args)
	local buf = args.buf

	-- Prevent execution if buffer already has content
	if vim.api.nvim_buf_line_count(buf) > 1 then
		return
	end

	local filename = vim.fn.expand("%:t")
	local filestem = vim.fn.expand("%:t:r")
	local extension = vim.fn.expand("%:e")
	local year = os.date("%Y")
	local date = os.date("%d/%m/%Y") -- Aquí recuperamos la fecha exacta

	-- Shared base header
	local template = {
		"/************************************************************************",
		" * Copyright (c) " .. year .. " Alvaro Cabrera Barrio",
		" * This code is licensed under MIT license (see LICENSE.txt for details)",
		" ************************************************************************/",
		"/**",
		" * @file " .. filename,
		" * @date " .. date,
		" * @brief Short description",
		" *",
		" * Longer description",
		" */",
		"",
	}

	-- Specific structure for headers (.hpp)
	if extension == "hpp" then
		table.insert(template, "#pragma once")
		table.insert(template, "")
		table.insert(template, "// --- Includes ---")
		table.insert(template, "")
		table.insert(template, "// --- Dependencies ---")
		table.insert(template, "")
		table.insert(template, "// --- External dependencies ---")
		table.insert(template, "")
		table.insert(template, "// --- STD ---")
		table.insert(template, "")
		table.insert(template, "// --- System ---")
		table.insert(template, "")

	-- Specific structure for source files (.cpp)
	elseif extension == "cpp" then
		-- Include the associated header first
		table.insert(template, "// --- Includes ---")
		table.insert(template, "")
		table.insert(template, "// --- Dependencies ---")
		table.insert(template, "")
		table.insert(template, "// --- External dependencies ---")
		table.insert(template, "")
		table.insert(template, "// --- STD ---")
		table.insert(template, "")
		table.insert(template, "// --- System ---")
		table.insert(template, "")
	end

	-- Write the lines to the buffer
	vim.api.nvim_buf_set_lines(buf, 0, -1, false, template)

	-- Safely move the cursor inside the namespace
	vim.schedule(function()
		local win = vim.fn.bufwinid(buf)
		if win ~= -1 then
			local line_to_go = #template - 1
			pcall(vim.api.nvim_win_set_cursor, win, { line_to_go, 0 })
		end
	end)
end

local group = vim.api.nvim_create_augroup("CppTemplateGroup", { clear = true })

-- Trigger for both .hpp and .cpp files
vim.api.nvim_create_autocmd({ "BufNewFile", "BufReadPost" }, {
	group = group,
	pattern = { "*.hpp", "*.cpp" },
	callback = function(args)
		local lines = vim.api.nvim_buf_get_lines(args.buf, 0, -1, false)
		if #lines <= 1 and (lines[1] == "" or lines[1] == nil) then
			insert_file_template(args)
		end
	end,
})
