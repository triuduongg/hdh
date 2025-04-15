# Chương trình này chỉ chạy được trên linux, shell zsh
## shell zsh:
### Kiểm tra:
```
zsh --version
```
### Cài đặt:
```
sudo apt update
sudo apt install zsh
```
### Kiểm tra shell mặc định:
```
echo $SHELL
```
### Đặt zsh làm shell mặc định rồi đăng nhập lại:
```
chsh -s $(which zsh) 
```
### Bật timestamp để dùng được chức năng time:
#### Mở .zshrc:
```
nano ~/.zshrc
```
#### Thêm lệnh vào rồi Ctrl+O, Enter, Ctrl+X:
```
setopt EXTENDED_HISTORY
setopt INC_APPEND_HISTORY
HISTFILE=~/.zsh_history
HISTSIZE=10000
SAVEHIST=10000
```
## Các gói ubuntu cần thiết:
### build-essential (gcc, g++ make):
#### Kiểm tra:
```
gcc --version
```
#### Cài đặt:
```
sudo apt update
sudo apt install build-essential
```
### GTK 3:
#### Kiểm tra: 
``` 
pkg-config --modversion gtk+-3.0
```
#### Cài đặt:
```
sudo apt update
sudo apt install libgtk-3-dev
```
## Biên dịch chương trình
### history.c
``` 
gcc -o h history.c
```
### gui.c
``` 
gcc -o gui gui.c `pkg-config --cflags --libs gtk+-3.0
```
## Chạy chương trình
### history.c
```
./h
```
### gui.c
```
./gui
```
