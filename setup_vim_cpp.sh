#!/bin/bash

echo "🔧 [1/5] 安装 Vim、CMake、Clangd、Git..."
sudo apt update
sudo apt install -y vim cmake clangd git curl wget build-essential unzip

echo "📦 [2/5] 安装 vim-plug 插件管理器..."
curl -fLo ~/.vim/autoload/plug.vim --create-dirs \
     https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim

echo "📄 [3/5] 写入 .vimrc 配置..."
cat > ~/.vimrc << 'EOF'
" 基础设置
set number
syntax on
set tabstop=4 shiftwidth=4 expandtab
set clipboard=unnamedplus
set mouse=a

" 插件管理
call plug#begin('~/.vim/plugged')

Plug 'preservim/nerdtree'          " 文件树
Plug 'vim-airline/vim-airline'     " 状态栏增强
Plug 'neoclide/coc.nvim', {'branch': 'release'}  " 自动补全 + LSP
Plug 'dense-analysis/ale'          " 实时语法检查
Plug 'Yggdroot/indentLine'         " 缩进线

call plug#end()

let g:coc_global_extensions = ['coc-clangd']
nmap <C-n> :NERDTreeToggle<CR>
EOF

echo "✅ [4/5] 初始化完成！现在请手动进入 Vim 并运行 :PlugInstall 安装插件。"
echo "👉 命令如下："
echo "   vim"
echo "   :PlugInstall"

echo "💡 提示：建议在项目目录下使用 cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON . 来生成 compile_commands.json"
