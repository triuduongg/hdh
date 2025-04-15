# Chương trình này chỉ chạy được trên linux
## Cài các gói ubuntu cần thiết:
### build-essential (gcc, g++ make):
```
sudo apt update
sudo apt install build-essential
```
### GTK 3:
```
sudo apt update
sudo apt install libgtk-3-dev
```
## Biên dịch chương trình
### history.c
``` gcc -o h history.c
### gui.c
``` gcc -o gui gui.c `pkg-config --cflags --libs gtk+-3.0
## Chạy chương trình
### history.c
``` ./h
### gui.c
``` ./gui
