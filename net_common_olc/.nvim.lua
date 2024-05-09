local Terminal = require("toggleterm.terminal").Terminal
local cmake_project = Terminal:new({
	cmd = "./build_and_run",
	close_on_exit = false,
	direction = "float",
	-- direction = "vertical",
	float_opts = {
		border = "double",
	},
	-- function to run on opening the terminal
	on_open = function(term)
		vim.cmd("startinsert!")
		vim.api.nvim_buf_set_keymap(term.bufnr, "n", "q", "<cmd>close<CR>", { noremap = true, silent = true })
	end,
	-- function to run on closing the terminal
	on_close = function(term)
		vim.cmd("startinsert!")
	end,
})
local run_project = Terminal:new({
	cmd = "./Run",
	close_on_exit = false,
	direction = "float",
	-- direction = "vertical",
	float_opts = {
		border = "double",
	},
	-- function to run on opening the terminal
	on_open = function(term)
		vim.cmd("startinsert!")
		vim.api.nvim_buf_set_keymap(term.bufnr, "n", "q", "<cmd>close<CR>", { noremap = true, silent = true })
	end,
	-- function to run on closing the terminal
	on_close = function(term)
		vim.cmd("startinsert!")
	end,
})

function Run_toggle()
	run_project:toggle()
end
function Cmake_toggle()
	cmake_project:toggle()
end
vim.api.nvim_set_keymap("n", "<leader>r", "<cmd>lua Run_toggle()<CR>", { noremap = true, silent = true })
vim.api.nvim_set_keymap("n", "<leader>m", "<cmd>lua Cmake_toggle()<CR>", { noremap = true, silent = true })
vim.api.nvim_set_keymap("n", "<leader>M", "<cmd>w | lua Cmake_toggle()<CR>", { noremap = true, silent = true })
