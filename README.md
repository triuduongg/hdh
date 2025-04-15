# Chương trình này chỉ chạy được trên linux, shell zsh
## Cài các gói ubuntu cần thiết:
### build-essential (gcc, g++ make):
`
sudo apt update
sudo apt install build-essential
`
#### Kiểm tra gói gcc:
` 
gcc --version
`
### GTK 3:
`
sudo apt update
sudo apt install libgtk-3-dev
`
#### Kiểm tra gói GTK 3: 
` 
pkg-config --modversion gtk+-3.0
`
## Thêm lệnh bật timestamp cho .zshrc:
`
setopt EXTENDED_HISTORY
setopt INC_APPEND_HISTORY
HISTFILE=~/.zsh_history
HISTSIZE=10000
SAVEHIST=10000
`
## Biên dịch chương trình
### history.c
` 
gcc -o h history.c
`
### gui.c
` 
gcc -o gui gui.c `pkg-config --cflags --libs gtk+-3.0
`
## Chạy chương trình
### history.c
`
./h
`
### gui.c
`
./gui
`
