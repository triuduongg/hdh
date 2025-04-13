#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>

// Hàm đọc newcommand từ input
char* read_new_command() {
    printf("Nhập newcommand (kết thúc bằng Enter): ");
    char* new_cmd = NULL;
    size_t len = 0;
    ssize_t read = getline(&new_cmd, &len, stdin);
    
    if (read == -1) {
        free(new_cmd);
        return NULL;
    }
    
    // Xóa ký tự newline ở cuối
    if (new_cmd[read - 1] == '\n') {
        new_cmd[read - 1] = '\0';
    }
    
    return new_cmd;
}

const char* get_history_filename() {
    char* shell = getenv("SHELL");
    if (shell == NULL) return ".custom_command_history";
    
    if (strstr(shell, "bash")) return ".bash_history";
    if (strstr(shell, "zsh")) return ".zsh_history";
    return ".custom_command_history";
}

char* get_history_path() {
    const char *home = getenv("HOME");
    if (!home) home = getpwuid(getuid())->pw_dir;
    
    const char *history_file = get_history_filename();
    char *path = malloc(strlen(home) + strlen(history_file) + 2);
    sprintf(path, "%s/%s", home, history_file);
    return path;
}

// Hàm lấy toàn bộ phần còn lại của command sau token đầu tiên
char* get_remaining_input(char* current_pos) {
    if (current_pos == NULL) return NULL;
    
    // Bỏ qua các khoảng trắng đầu
    while (*current_pos == ' ') current_pos++;
    
    if (*current_pos == '\0') return NULL;
    
    return current_pos;
}

// Hàm đếm tổng số lệnh trong history file
int count_total_commands() {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        free(path);
        return 0;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 0;

    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') count++;
    }

    free(line);
    fclose(file);
    free(path);
    return count;
}

void show_help() {
    printf("\n  LỆNH        | THAM SỐ\n");
    printf("  view  : Xem | không tham số : Toàn bộ lịch sử lệnh\n");
    printf("  run   : Chạy| [num]         : Lệnh cụ thể\n");
    printf("  clear : Xóa | [start] [end] : Các lệnh trong phạm vi\n");
    printf("  save  : Lưu | with [keyword]: Các lệnh chứa từ khóa\n");
    printf("  change: Sửa | but [keyword] : Các lệnh không chứa từ khóa\n");
    printf("  time  :     | first [num]   : Số lệnh đầu tiên\n");
    printf("              | last [num]    : Số lệnh cuối cùng\n");
    printf("\n  help, quit: Hướng dẫn, Thoát\n");
}

void show_full_history() {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;

    printf("\n--- TOÀN BỘ LỊCH SỬ LỆNH ---\n");
    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            printf("%5d: %s", count++, line);
        }
    }

    free(line);
    fclose(file);
    free(path);
}

void show_single_command(int num) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int found = 0;

    while (getline(&line, &len, file) != -1 && !found) {
        if (line[0] != '#') {
            if (count == num) {
                printf("\n--- LỆNH SỐ %d ---\n", num);
                printf("%s", line);
                found = 1;
            }
            count++;
        }
    }

    if (!found) {
        printf("Không tìm thấy lệnh số %d\n", num);
    }

    free(line);
    fclose(file);
    free(path);
}

void show_history_range(int start, int end) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int total_commands = 0;

    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') total_commands++;
    }
    rewind(file);

    if (start < 1) start = 1;
    if (end > total_commands) end = total_commands;
    if (start > end) {
        printf("Phạm vi không hợp lệ.\n");
        free(line);
        fclose(file);
        free(path);
        return;
    }

    printf("\n--- LỊCH SỬ LỆNH (%d-%d/%d) ---\n", start, end, total_commands);
    while (getline(&line, &len, file) != -1 && count <= end) {
        if (line[0] != '#') {
            if (count >= start) {
                printf("%5d: %s", count, line);
            }
            count++;
        }
    }

    free(line);
    fclose(file);
    free(path);
}

void show_filtered_commands(int filter_type, const char *keyword) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int found = 0;

    printf("\n--- KẾT QUẢ LỌC LỆNH (%s chứa '%s') ---\n", 
           filter_type ? "CÓ" : "KHÔNG", keyword);
    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            line[strcspn(line, "\n")] = '\0'; // Xóa ký tự newline
            int contains = (strstr(line, keyword) != NULL);
            if ((filter_type && contains) || (!filter_type && !contains)) {
                printf("%5d: %s\n", count, line);
                found = 1;
            }
            count++;
        }
    }

    if (!found) {
        printf("Không tìm thấy lệnh nào phù hợp\n");
    }

    free(line);
    fclose(file);
    free(path);
}

void execute_command(const char *cmd) {
    printf("\nĐang chạy: %s\n", cmd);
    int result = system(cmd);
    if (result == -1) {
        perror("Lỗi khi chạy lệnh");
    }
}

void run_all_commands() {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;

    printf("\n--- CHẠY TOÀN BỘ LỊCH SỬ LỆNH ---\n");
    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            printf("\n[%d] ", count++);
            execute_command(line);
        }
    }

    free(line);
    fclose(file);
    free(path);
}

void run_single_command(int num) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int found = 0;

    while (getline(&line, &len, file) != -1 && !found) {
        if (line[0] != '#') {
            if (count == num) {
                printf("\n--- CHẠY LỆNH SỐ %d ---\n", num);
                execute_command(line);
                found = 1;
            }
            count++;
        }
    }

    if (!found) {
        printf("Không tìm thấy lệnh số %d\n", num);
    }

    free(line);
    fclose(file);
    free(path);
}

void run_command_range(int start, int end) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int total_commands = 0;

    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') total_commands++;
    }
    rewind(file);

    if (start < 1) start = 1;
    if (end > total_commands) end = total_commands;
    if (start > end) {
        printf("Phạm vi không hợp lệ.\n");
        free(line);
        fclose(file);
        free(path);
        return;
    }

    printf("\n--- CHẠY CÁC LỆNH TỪ %d ĐẾN %d ---\n", start, end);
    while (getline(&line, &len, file) != -1 && count <= end) {
        if (line[0] != '#') {
            if (count >= start) {
                printf("\n[%d] ", count);
                execute_command(line);
            }
            count++;
        }
    }

    free(line);
    fclose(file);
    free(path);
}

void run_filtered_commands(int filter_type, const char *keyword) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int found = 0;

    printf("\n--- CHẠY LỆNH (%s chứa '%s') ---\n", 
           filter_type ? "CÓ" : "KHÔNG", keyword);
    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            line[strcspn(line, "\n")] = '\0';
            int contains = (strstr(line, keyword) != NULL);
            if ((filter_type && contains) || (!filter_type && !contains)) {
                printf("\n[%d] ", count);
                execute_command(line);
                found = 1;
            }
            count++;
        }
    }

    if (!found) {
        printf("Không tìm thấy lệnh nào phù hợp\n");
    }

    free(line);
    fclose(file);
    free(path);
}

void delete_command(int num) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *temp_path = malloc(strlen(path) + 5);
    sprintf(temp_path, "%s.tmp", path);
    FILE *temp_file = fopen(temp_path, "w");
    if (!temp_file) {
        perror("Không thể tạo file tạm");
        fclose(file);
        free(path);
        free(temp_path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int deleted = 0;

    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            if (count == num) {
                fprintf(temp_file, "# Đã xóa\n");
                deleted = 1;
            } else {
                fprintf(temp_file, "%s", line);
            }
            count++;
        } else {
            fprintf(temp_file, "%s", line);
        }
    }

    free(line);
    fclose(file);
    fclose(temp_file);

    if (deleted) {
        if (rename(temp_path, path) != 0) {
            perror("Không thể cập nhật file lịch sử");
        } else {
            printf("Đã xóa lệnh số %d\n", num);
        }
    } else {
        printf("Không tìm thấy lệnh số %d để xóa\n", num);
        remove(temp_path);
    }

    free(path);
    free(temp_path);
}

void delete_command_range(int start, int end) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *temp_path = malloc(strlen(path) + 5);
    sprintf(temp_path, "%s.tmp", path);
    FILE *temp_file = fopen(temp_path, "w");
    if (!temp_file) {
        perror("Không thể tạo file tạm");
        fclose(file);
        free(path);
        free(temp_path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int deleted = 0;

    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            if (count >= start && count <= end) {
                fprintf(temp_file, "# Đã xóa\n");
                deleted = 1;
            } else {
                fprintf(temp_file, "%s", line);
            }
            count++;
        } else {
            fprintf(temp_file, "%s", line);
        }
    }

    free(line);
    fclose(file);
    fclose(temp_file);

    if (deleted) {
        if (rename(temp_path, path) != 0) {
            perror("Không thể cập nhật file lịch sử");
        } else {
            printf("Đã xóa các lệnh từ %d đến %d\n", start, end);
        }
    } else {
        printf("Không tìm thấy lệnh trong khoảng %d đến %d để xóa\n", start, end);
        remove(temp_path);
    }

    free(path);
    free(temp_path);
}

void delete_filtered_commands(int filter_type, const char *keyword) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *temp_path = malloc(strlen(path) + 5);
    sprintf(temp_path, "%s.tmp", path);
    FILE *temp_file = fopen(temp_path, "w");
    if (!temp_file) {
        perror("Không thể tạo file tạm");
        fclose(file);
        free(path);
        free(temp_path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int deleted = 0;

    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            line[strcspn(line, "\n")] = '\0';
            int contains = (strstr(line, keyword) != NULL);
            if ((filter_type && contains) || (!filter_type && !contains)) {
                fprintf(temp_file, "# Đã xóa\n");
                deleted++;
            } else {
                fprintf(temp_file, "%s\n", line);
            }
            count++;
        } else {
            fprintf(temp_file, "%s", line);
        }
    }

    free(line);
    fclose(file);
    fclose(temp_file);

    if (deleted > 0) {
        if (rename(temp_path, path) != 0) {
            perror("Không thể cập nhật file lịch sử");
        } else {
            printf("Đã xóa %d lệnh (%s chứa '%s')\n", 
                  deleted, filter_type ? "CÓ" : "KHÔNG", keyword);
        }
    } else {
        printf("Không tìm thấy lệnh nào phù hợp để xóa\n");
        remove(temp_path);
    }

    free(path);
    free(temp_path);
}

void clear_shell_history() {
    char *path = get_history_path();
    
    if (remove(path) == 0) {
        printf("Đã xóa toàn bộ lịch sử lệnh.\n");
        
        FILE *file = fopen(path, "w");
        if (file) fclose(file);
    } else {
        perror("Không thể xóa lịch sử");
    }
    
    free(path);
}

void save_all_commands() {
    char *path = get_history_path();
    FILE *history_file = fopen(path, "r");
    if (!history_file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    FILE *output_file = fopen("history.txt", "w");
    if (!output_file) {
        printf("Không thể tạo file history.txt\n");
        fclose(history_file);
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;

    while (getline(&line, &len, history_file) != -1) {
        if (line[0] != '#') {
            fprintf(output_file, "%s", line);
            count++;
        }
    }

    fclose(history_file);
    fclose(output_file);
    free(line);
    free(path);
    printf("Đã lưu toàn bộ lịch sử lệnh vào file history.txt\n");
}

void save_single_command(int num) {
    char *path = get_history_path();
    FILE *history_file = fopen(path, "r");
    if (!history_file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    FILE *output_file = fopen("history.txt", "w");
    if (!output_file) {
        printf("Không thể tạo file history.txt\n");
        fclose(history_file);
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int found = 0;

    while (getline(&line, &len, history_file) != -1 && !found) {
        if (line[0] != '#') {
            if (count == num) {
                fprintf(output_file, "%s", line);
                found = 1;
            }
            count++;
        }
    }

    fclose(history_file);
    fclose(output_file);
    free(line);
    free(path);

    if (found) {
        printf("Đã lưu lệnh số %d vào file history.txt\n", num);
    } else {
        printf("Không tìm thấy lệnh số %d\n", num);
        remove("history.txt");
    }
}

void save_command_range(int start, int end) {
    char *path = get_history_path();
    FILE *history_file = fopen(path, "r");
    if (!history_file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    FILE *output_file = fopen("history.txt", "w");
    if (!output_file) {
        printf("Không thể tạo file history.txt\n");
        fclose(history_file);
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int total_commands = 0;

    while (getline(&line, &len, history_file) != -1) {
        if (line[0] != '#') total_commands++;
    }
    rewind(history_file);

    if (start < 1) start = 1;
    if (end > total_commands) end = total_commands;
    if (start > end) {
        printf("Phạm vi không hợp lệ.\n");
        free(line);
        fclose(history_file);
        fclose(output_file);
        free(path);
        remove("history.txt");
        return;
    }

    int saved = 0;
    while (getline(&line, &len, history_file) != -1 && count <= end) {
        if (line[0] != '#') {
            if (count >= start) {
                fprintf(output_file, "%s", line);
                saved++;
            }
            count++;
        }
    }

    fclose(history_file);
    fclose(output_file);
    free(line);
    free(path);

    if (saved > 0) {
        printf("Đã lưu %d lệnh (từ %d đến %d) vào file history.txt\n", saved, start, end);
    } else {
        printf("Không tìm thấy lệnh trong khoảng %d đến %d\n", start, end);
        remove("history.txt");
    }
}

void save_filtered_commands(int filter_type, const char *keyword) {
    char *path = get_history_path();
    FILE *history_file = fopen(path, "r");
    if (!history_file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    FILE *output_file = fopen("history.txt", "w");
    if (!output_file) {
        printf("Không thể tạo file history.txt\n");
        fclose(history_file);
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int saved = 0;

    while (getline(&line, &len, history_file) != -1) {
        if (line[0] != '#') {
            line[strcspn(line, "\n")] = '\0';
            int contains = (strstr(line, keyword) != NULL);
            if ((filter_type && contains) || (!filter_type && !contains)) {
                fprintf(output_file, "%s\n", line);
                saved++;
            }
            count++;
        }
    }

    fclose(history_file);
    fclose(output_file);
    free(line);
    free(path);

    if (saved > 0) {
        printf("Đã lưu %d lệnh (%s chứa '%s') vào file history.txt\n", 
              saved, filter_type ? "CÓ" : "KHÔNG", keyword);
    } else {
        printf("Không tìm thấy lệnh nào phù hợp\n");
        remove("history.txt");
    }
}

// Hiển thị N lệnh đầu tiên
void show_first_commands(int n) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int total = count_total_commands();

    if (n > total) n = total;
    if (n < 1) n = 1;

    printf("\n--- %d LỆNH ĐẦU TIÊN ---\n", n);
    while (getline(&line, &len, file) != -1 && count <= n) {
        if (line[0] != '#') {
            printf("%5d: %s", count++, line);
        }
    }

    free(line);
    fclose(file);
    free(path);
}

// Hiển thị N lệnh cuối cùng
void show_last_commands(int n) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int total = count_total_commands();
    int start = total - n + 1;
    int count = 1;

    if (n > total) n = total;
    if (n < 1) n = 1;
    if (start < 1) start = 1;

    printf("\n--- %d LỆNH CUỐI CÙNG ---\n", n);
    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            if (count >= start) {
                printf("%5d: %s", count, line);
            }
            count++;
        }
    }

    free(line);
    fclose(file);
    free(path);
}

// Chạy N lệnh đầu tiên
void run_first_commands(int n) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int total = count_total_commands();

    if (n > total) n = total;
    if (n < 1) n = 1;

    printf("\n--- CHẠY %d LỆNH ĐẦU TIÊN ---\n", n);
    while (getline(&line, &len, file) != -1 && count <= n) {
        if (line[0] != '#') {
            printf("\n[%d] ", count++);
            execute_command(line);
        }
    }

    free(line);
    fclose(file);
    free(path);
}

// Chạy N lệnh cuối cùng
void run_last_commands(int n) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int total = count_total_commands();
    int start = total - n + 1;
    int count = 1;

    if (n > total) n = total;
    if (n < 1) n = 1;
    if (start < 1) start = 1;

    printf("\n--- CHẠY %d LỆNH CUỐI CÙNG ---\n", n);
    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            if (count >= start) {
                printf("\n[%d] ", count);
                execute_command(line);
            }
            count++;
        }
    }

    free(line);
    fclose(file);
    free(path);
}

// Xóa N lệnh đầu tiên
void delete_first_commands(int n) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *temp_path = malloc(strlen(path) + 5);
    sprintf(temp_path, "%s.tmp", path);
    FILE *temp_file = fopen(temp_path, "w");
    if (!temp_file) {
        perror("Không thể tạo file tạm");
        fclose(file);
        free(path);
        free(temp_path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int total = count_total_commands();
    int deleted = 0;

    if (n > total) n = total;
    if (n < 1) n = 1;

    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            if (count <= n) {
                fprintf(temp_file, "# Đã xóa\n");
                deleted++;
            } else {
                fprintf(temp_file, "%s", line);
            }
            count++;
        } else {
            fprintf(temp_file, "%s", line);
        }
    }

    free(line);
    fclose(file);
    fclose(temp_file);

    if (deleted > 0) {
        if (rename(temp_path, path) != 0) {
            perror("Không thể cập nhật file lịch sử");
        } else {
            printf("Đã xóa %d lệnh đầu tiên\n", deleted);
        }
    } else {
        printf("Không có lệnh nào để xóa\n");
        remove(temp_path);
    }

    free(path);
    free(temp_path);
}

// Xóa N lệnh cuối cùng
void delete_last_commands(int n) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    char *temp_path = malloc(strlen(path) + 5);
    sprintf(temp_path, "%s.tmp", path);
    FILE *temp_file = fopen(temp_path, "w");
    if (!temp_file) {
        perror("Không thể tạo file tạm");
        fclose(file);
        free(path);
        free(temp_path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int total = count_total_commands();
    int start = total - n + 1;
    int deleted = 0;

    if (n > total) n = total;
    if (n < 1) n = 1;
    if (start < 1) start = 1;

    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            if (count >= start) {
                fprintf(temp_file, "# Đã xóa\n");
                deleted++;
            } else {
                fprintf(temp_file, "%s", line);
            }
            count++;
        } else {
            fprintf(temp_file, "%s", line);
        }
    }

    free(line);
    fclose(file);
    fclose(temp_file);

    if (deleted > 0) {
        if (rename(temp_path, path) != 0) {
            perror("Không thể cập nhật file lịch sử");
        } else {
            printf("Đã xóa %d lệnh cuối cùng\n", deleted);
        }
    } else {
        printf("Không có lệnh nào để xóa\n");
        remove(temp_path);
    }

    free(path);
    free(temp_path);
}

// Lưu N lệnh đầu tiên vào file
void save_first_commands(int n) {
    char *path = get_history_path();
    FILE *history_file = fopen(path, "r");
    if (!history_file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    FILE *output_file = fopen("history.txt", "w");
    if (!output_file) {
        printf("Không thể tạo file history.txt\n");
        fclose(history_file);
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int total = count_total_commands();
    int saved = 0;

    if (n > total) n = total;
    if (n < 1) n = 1;

    while (getline(&line, &len, history_file) != -1 && count <= n) {
        if (line[0] != '#') {
            fprintf(output_file, "%s", line);
            saved++;
            count++;
        }
    }

    fclose(history_file);
    fclose(output_file);
    free(line);
    free(path);

    if (saved > 0) {
        printf("Đã lưu %d lệnh đầu tiên vào file history.txt\n", saved);
    } else {
        printf("Không có lệnh nào để lưu\n");
        remove("history.txt");
    }
}

// Lưu N lệnh cuối cùng vào file
void save_last_commands(int n) {
    char *path = get_history_path();
    FILE *history_file = fopen(path, "r");
    if (!history_file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        return;
    }

    FILE *output_file = fopen("history.txt", "w");
    if (!output_file) {
        printf("Không thể tạo file history.txt\n");
        fclose(history_file);
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int total = count_total_commands();
    int start = total - n + 1;
    int saved = 0;

    if (n > total) n = total;
    if (n < 1) n = 1;
    if (start < 1) start = 1;

    while (getline(&line, &len, history_file) != -1) {
        if (line[0] != '#') {
            if (count >= start) {
                fprintf(output_file, "%s", line);
                saved++;
            }
            count++;
        }
    }

    fclose(history_file);
    fclose(output_file);
    free(line);
    free(path);

    if (saved > 0) {
        printf("Đã lưu %d lệnh cuối cùng vào file history.txt\n", saved);
    } else {
        printf("Không có lệnh nào để lưu\n");
        remove("history.txt");
    }
}

// Thay đổi toàn bộ lệnh thành command mới
void change_all_commands() {
    char* new_cmd = read_new_command();
    if (new_cmd == NULL || *new_cmd == '\0') {
        printf("Không có command mới\n");
        free(new_cmd);
        return;
    }

    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        free(new_cmd);
        return;
    }

    char *temp_path = malloc(strlen(path) + 5);
    sprintf(temp_path, "%s.tmp", path);
    FILE *temp_file = fopen(temp_path, "w");
    if (!temp_file) {
        perror("Không thể tạo file tạm");
        fclose(file);
        free(path);
        free(temp_path);
        free(new_cmd);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int changed = 0;

    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            fprintf(temp_file, "%s\n", new_cmd);
            changed++;
        } else {
            fprintf(temp_file, "%s", line);
        }
    }

    free(line);
    fclose(file);
    fclose(temp_file);

    if (changed > 0) {
        if (rename(temp_path, path) != 0) {
            perror("Không thể cập nhật file lịch sử");
        } else {
            printf("Đã thay đổi %d lệnh thành '%s'\n", changed, new_cmd);
        }
    } else {
        printf("Không có lệnh nào để thay đổi\n");
        remove(temp_path);
    }

    free(path);
    free(temp_path);
    free(new_cmd);
}

// Thay đổi lệnh số num thành command mới
void change_single_command(int num) {
    char* new_cmd = read_new_command();
    if (new_cmd == NULL || *new_cmd == '\0') {
        printf("Không có command mới\n");
        free(new_cmd);
        return;
    }

    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        free(new_cmd);
        return;
    }

    char *temp_path = malloc(strlen(path) + 5);
    sprintf(temp_path, "%s.tmp", path);
    FILE *temp_file = fopen(temp_path, "w");
    if (!temp_file) {
        perror("Không thể tạo file tạm");
        fclose(file);
        free(path);
        free(temp_path);
        free(new_cmd);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int changed = 0;

    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            if (count == num) {
                fprintf(temp_file, "%s\n", new_cmd);
                changed = 1;
            } else {
                fprintf(temp_file, "%s", line);
            }
            count++;
        } else {
            fprintf(temp_file, "%s", line);
        }
    }

    free(line);
    fclose(file);
    fclose(temp_file);

    if (changed) {
        if (rename(temp_path, path) != 0) {
            perror("Không thể cập nhật file lịch sử");
        } else {
            printf("Đã thay đổi lệnh số %d thành '%s'\n", num, new_cmd);
        }
    } else {
        printf("Không tìm thấy lệnh số %d để thay đổi\n", num);
        remove(temp_path);
    }

    free(path);
    free(temp_path);
    free(new_cmd);
}

// Thay đổi các lệnh từ start đến end thành command mới
void change_command_range(int start, int end) {
    char* new_cmd = read_new_command();
    if (new_cmd == NULL || *new_cmd == '\0') {
        printf("Không có command mới\n");
        free(new_cmd);
        return;
    }

    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        free(new_cmd);
        return;
    }

    char *temp_path = malloc(strlen(path) + 5);
    sprintf(temp_path, "%s.tmp", path);
    FILE *temp_file = fopen(temp_path, "w");
    if (!temp_file) {
        perror("Không thể tạo file tạm");
        fclose(file);
        free(path);
        free(temp_path);
        free(new_cmd);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int total_commands = 0;
    int changed = 0;

    // Đếm tổng số lệnh
    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') total_commands++;
    }
    rewind(file);

    if (start < 1) start = 1;
    if (end > total_commands) end = total_commands;
    if (start > end) {
        printf("Phạm vi không hợp lệ.\n");
        free(line);
        fclose(file);
        fclose(temp_file);
        free(path);
        free(temp_path);
        free(new_cmd);
        remove(temp_path);
        return;
    }

    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            if (count >= start && count <= end) {
                fprintf(temp_file, "%s\n", new_cmd);
                changed++;
            } else {
                fprintf(temp_file, "%s", line);
            }
            count++;
        } else {
            fprintf(temp_file, "%s", line);
        }
    }

    free(line);
    fclose(file);
    fclose(temp_file);

    if (changed > 0) {
        if (rename(temp_path, path) != 0) {
            perror("Không thể cập nhật file lịch sử");
        } else {
            printf("Đã thay đổi %d lệnh (từ %d đến %d) thành '%s'\n", changed, start, end, new_cmd);
        }
    } else {
        printf("Không tìm thấy lệnh trong khoảng %d đến %d để thay đổi\n", start, end);
        remove(temp_path);
    }

    free(path);
    free(temp_path);
    free(new_cmd);
}

// Thay đổi các lệnh theo điều kiện có/không chứa keyword
void change_filtered_commands(int filter_type, const char *keyword) {
    char* new_cmd = read_new_command();
    if (new_cmd == NULL || *new_cmd == '\0') {
        printf("Không có command mới\n");
        free(new_cmd);
        return;
    }

    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        free(new_cmd);
        return;
    }

    char *temp_path = malloc(strlen(path) + 5);
    sprintf(temp_path, "%s.tmp", path);
    FILE *temp_file = fopen(temp_path, "w");
    if (!temp_file) {
        perror("Không thể tạo file tạm");
        fclose(file);
        free(path);
        free(temp_path);
        free(new_cmd);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int changed = 0;

    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            line[strcspn(line, "\n")] = '\0';
            int contains = (strstr(line, keyword) != NULL);
            if ((filter_type && contains) || (!filter_type && !contains)) {
                fprintf(temp_file, "%s\n", new_cmd);
                changed++;
            } else {
                fprintf(temp_file, "%s\n", line);
            }
            count++;
        } else {
            fprintf(temp_file, "%s", line);
        }
    }

    free(line);
    fclose(file);
    fclose(temp_file);

    if (changed > 0) {
        if (rename(temp_path, path) != 0) {
            perror("Không thể cập nhật file lịch sử");
        } else {
            printf("Đã thay đổi %d lệnh (%s chứa '%s') thành '%s'\n", 
                  changed, filter_type ? "CÓ" : "KHÔNG", keyword, new_cmd);
        }
    } else {
        printf("Không tìm thấy lệnh nào phù hợp để thay đổi\n");
        remove(temp_path);
    }

    free(path);
    free(temp_path);
    free(new_cmd);
}

// Thay đổi num lệnh đầu tiên thành command mới
void change_first_commands(int num) {
    char* new_cmd = read_new_command();
    if (new_cmd == NULL || *new_cmd == '\0') {
        printf("Không có command mới\n");
        free(new_cmd);
        return;
    }

    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        free(new_cmd);
        return;
    }

    char *temp_path = malloc(strlen(path) + 5);
    sprintf(temp_path, "%s.tmp", path);
    FILE *temp_file = fopen(temp_path, "w");
    if (!temp_file) {
        perror("Không thể tạo file tạm");
        fclose(file);
        free(path);
        free(temp_path);
        free(new_cmd);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int total = count_total_commands();
    int changed = 0;

    if (num > total) num = total;
    if (num < 1) num = 1;

    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            if (count <= num) {
                fprintf(temp_file, "%s\n", new_cmd);
                changed++;
            } else {
                fprintf(temp_file, "%s", line);
            }
            count++;
        } else {
            fprintf(temp_file, "%s", line);
        }
    }

    free(line);
    fclose(file);
    fclose(temp_file);

    if (changed > 0) {
        if (rename(temp_path, path) != 0) {
            perror("Không thể cập nhật file lịch sử");
        } else {
            printf("Đã thay đổi %d lệnh đầu tiên thành '%s'\n", changed, new_cmd);
        }
    } else {
        printf("Không có lệnh nào để thay đổi\n");
        remove(temp_path);
    }

    free(path);
    free(temp_path);
    free(new_cmd);
}

// Thay đổi num lệnh cuối cùng thành command mới
void change_last_commands(int num) {
    char* new_cmd = read_new_command();
    if (new_cmd == NULL || *new_cmd == '\0') {
        printf("Không có command mới\n");
        free(new_cmd);
        return;
    }

    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không tìm thấy file lịch sử hoặc không thể đọc file.\n");
        free(path);
        free(new_cmd);
        return;
    }

    char *temp_path = malloc(strlen(path) + 5);
    sprintf(temp_path, "%s.tmp", path);
    FILE *temp_file = fopen(temp_path, "w");
    if (!temp_file) {
        perror("Không thể tạo file tạm");
        fclose(file);
        free(path);
        free(temp_path);
        free(new_cmd);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int total = count_total_commands();
    int start = total - num + 1;
    int changed = 0;

    if (num > total) num = total;
    if (num < 1) num = 1;
    if (start < 1) start = 1;

    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            if (count >= start) {
                fprintf(temp_file, "%s\n", new_cmd);
                changed++;
            } else {
                fprintf(temp_file, "%s", line);
            }
            count++;
        } else {
            fprintf(temp_file, "%s", line);
        }
    }

    free(line);
    fclose(file);
    fclose(temp_file);

    if (changed > 0) {
        if (rename(temp_path, path) != 0) {
            perror("Không thể cập nhật file lịch sử");
        } else {
            printf("Đã thay đổi %d lệnh cuối cùng thành '%s'\n", changed, new_cmd);
        }
    } else {
        printf("Không có lệnh nào để thay đổi\n");
        remove(temp_path);
    }

    free(path);
    free(temp_path);
    free(new_cmd);
}

int is_number(const char *str) {
    while (*str) {
        if (!isdigit(*str)) return 0;
        str++;
    }
    return 1;
}

// Hàm đọc thời gian thực thi từ file lịch sử
void get_command_timestamp(const char *line, char *timestamp_buf) {
    // Xử lý cả định dạng Bash (#) và Zsh (:)
    if (line[0] == ':') {
        // Xử lý định dạng Zsh ": timestamp:duration;command"
        char *colon = strchr(line + 2, ':');
        if (colon) {
            time_t rawtime = atol(line + 2);  // Bỏ qua ": "
            
            // Thêm phần xử lý múi giờ
            struct tm *timeinfo = localtime(&rawtime);
            timeinfo->tm_hour += 7;  // Điều chỉnh sang múi giờ Việt Nam (UTC+7)
            mktime(timeinfo);  // Chuẩn hóa lại thời gian
            
            strftime(timestamp_buf, 20, "%H:%M-%d/%m/%Y", timeinfo);
            return;
        }
    }
    else if (line[0] == '#') {
        // Xử lý định dạng Bash
        time_t rawtime = atol(line + 1);
        struct tm *timeinfo = localtime(&rawtime);
        strftime(timestamp_buf, 20, "%H:%M-%d/%m/%Y", timeinfo);
        return;
    }
    strcpy(timestamp_buf, "Unknown");
}

// Hiển thị thời gian của tất cả lệnh
void show_all_timestamps() {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không thể mở file lịch sử\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    char timestamp[20];

    printf("\n--- THỜI GIAN THỰC THI TẤT CẢ LỆNH ---\n");
    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            get_command_timestamp(line, timestamp);
            printf("%5d: %s - %s", count, timestamp, line);
            count++;
        }
    }

    free(line);
    fclose(file);
    free(path);
}

// Hiển thị thời gian của lệnh cụ thể
void show_single_timestamp(int num) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không thể mở file lịch sử\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int found = 0;
    char timestamp[20];

    while (getline(&line, &len, file) != -1 && !found) {
        if (line[0] != '#') {
            if (count == num) {
                get_command_timestamp(line, timestamp);
                printf("\n--- THỜI GIAN LỆNH SỐ %d ---\n", num);
                printf("%s - %s", timestamp, line);
                found = 1;
            }
            count++;
        }
    }

    if (!found) {
        printf("Không tìm thấy lệnh số %d\n", num);
    }

    free(line);
    fclose(file);
    free(path);
}

// Hiển thị thời gian theo phạm vi
void show_timestamp_range(int start, int end) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không thể mở file lịch sử\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    char timestamp[20];
    int total_commands = count_total_commands();

    if (start < 1) start = 1;
    if (end > total_commands) end = total_commands;
    if (start > end) {
        printf("Phạm vi không hợp lệ\n");
        free(line);
        fclose(file);
        free(path);
        return;
    }

    printf("\n--- THỜI GIAN LỆNH TỪ %d ĐẾN %d ---\n", start, end);
    while (getline(&line, &len, file) != -1 && count <= end) {
        if (line[0] != '#') {
            if (count >= start) {
                get_command_timestamp(line, timestamp);
                printf("%5d: %s - %s", count, timestamp, line);
            }
            count++;
        }
    }

    free(line);
    fclose(file);
    free(path);
}

// Hiển thị thời gian theo từ khóa
void show_filtered_timestamps(int filter_type, const char *keyword) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không thể mở file lịch sử\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int found = 0;
    char timestamp[20];

    printf("\n--- THỜI GIAN LỆNH (%s CHỨA '%s') ---\n", 
           filter_type ? "CÓ" : "KHÔNG", keyword);
    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            line[strcspn(line, "\n")] = '\0';
            int contains = (strstr(line, keyword) != NULL);
            if ((filter_type && contains) || (!filter_type && !contains)) {
                get_command_timestamp(line, timestamp);
                printf("%5d: %s - %s\n", count, timestamp, line);
                found = 1;
            }
            count++;
        }
    }

    if (!found) {
        printf("Không tìm thấy lệnh nào phù hợp\n");
    }

    free(line);
    fclose(file);
    free(path);
}

// Hiển thị thời gian của N lệnh đầu
void show_first_timestamps(int n) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không thể mở file lịch sử\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int total = count_total_commands();
    char timestamp[20];

    if (n > total) n = total;
    if (n < 1) n = 1;

    printf("\n--- THỜI GIAN %d LỆNH ĐẦU TIÊN ---\n", n);
    while (getline(&line, &len, file) != -1 && count <= n) {
        if (line[0] != '#') {
            get_command_timestamp(line, timestamp);
            printf("%5d: %s - %s", count, timestamp, line);
            count++;
        }
    }

    free(line);
    fclose(file);
    free(path);
}

// Hiển thị thời gian của N lệnh cuối
void show_last_timestamps(int n) {
    char *path = get_history_path();
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Không thể mở file lịch sử\n");
        free(path);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    int count = 1;
    int total = count_total_commands();
    int start = total - n + 1;
    char timestamp[20];

    if (n > total) n = total;
    if (n < 1) n = 1;
    if (start < 1) start = 1;

    printf("\n--- THỜI GIAN %d LỆNH CUỐI CÙNG ---\n", n);
    while (getline(&line, &len, file) != -1) {
        if (line[0] != '#') {
            if (count >= start) {
                get_command_timestamp(line, timestamp);
                printf("%5d: %s - %s", count, timestamp, line);
            }
            count++;
        }
    }

    free(line);
    fclose(file);
    free(path);
}

int main() {
    char *cmd = NULL;
    size_t len = 0;
    
    printf("COMMAND HISTORY MANAGER (Gõ 'help' để xem hướng dẫn)\n");
    
    while (1) {
        printf("\n> ");
        ssize_t read = getline(&cmd, &len, stdin);
        
        if (read == -1) break;
        
        cmd[strcspn(cmd, "\n")] = '\0';
        
        if (strlen(cmd) == 0) continue;
        
        char *token = strtok(cmd, " ");
        if (token == NULL) continue;
        
        if (strcmp(token, "help") == 0) {
            show_help();
        }
        else if (strcmp(token, "view") == 0) {
            char *arg1 = strtok(NULL, " ");
            if (arg1 == NULL) {
                show_full_history();
                continue;
            }
            
            if (strcmp(arg1, "with") == 0 || strcmp(arg1, "but") == 0) {
                int filter_type = (strcmp(arg1, "with") == 0) ? 1 : 0;
                char *keyword = get_remaining_input(arg1 + strlen(arg1) + 1);
                if (keyword == NULL || *keyword == '\0') {
                    printf("Thiếu từ khóa\n");
                    continue;
                }
                show_filtered_commands(filter_type, keyword);
            }
            else if (strcmp(arg1, "first") == 0) {
                char *num_str = strtok(NULL, " ");
                if (num_str == NULL || !is_number(num_str)) {
                    printf("Thiếu số lượng hoặc số không hợp lệ\n");
                    continue;
                }
                show_first_commands(atoi(num_str));
            }
            else if (strcmp(arg1, "last") == 0) {
                char *num_str = strtok(NULL, " ");
                if (num_str == NULL || !is_number(num_str)) {
                    printf("Thiếu số lượng hoặc số không hợp lệ\n");
                    continue;
                }
                show_last_commands(atoi(num_str));
            }
            else if (is_number(arg1)) {
                char *arg2 = strtok(NULL, " ");
                if (arg2 == NULL) {
                    show_single_command(atoi(arg1));
                }
                else if (is_number(arg2)) {
                    show_history_range(atoi(arg1), atoi(arg2));
                }
                else {
                    printf("Sử dụng: view [num] hoặc view [start] [end] hoặc view y [keyword] hoặc view n [keyword] hoặc view first [num] hoặc view last [num] hoặc view\n");
                }
            }
            else {
                printf("Sử dụng: view [num] hoặc view [start] [end] hoặc view y [keyword] hoặc view n [keyword] hoặc view first [num] hoặc view last [num] hoặc view\n");
            }
        }
        else if (strcmp(token, "run") == 0) {
            char *arg1 = strtok(NULL, " ");
            if (arg1 == NULL) {
                run_all_commands();
                continue;
            }
            
            if (strcmp(arg1, "with") == 0 || strcmp(arg1, "but") == 0) {
                int filter_type = (strcmp(arg1, "with") == 0) ? 1 : 0;
                char *keyword = get_remaining_input(arg1 + strlen(arg1) + 1);
                if (keyword == NULL || *keyword == '\0') {
                    printf("Thiếu từ khóa\n");
                    continue;
                }
                run_filtered_commands(filter_type, keyword);
            }
            else if (strcmp(arg1, "first") == 0) {
                char *num_str = strtok(NULL, " ");
                if (num_str == NULL || !is_number(num_str)) {
                    printf("Thiếu số lượng hoặc số không hợp lệ\n");
                    continue;
                }
                run_first_commands(atoi(num_str));
            }
            else if (strcmp(arg1, "last") == 0) {
                char *num_str = strtok(NULL, " ");
                if (num_str == NULL || !is_number(num_str)) {
                    printf("Thiếu số lượng hoặc số không hợp lệ\n");
                    continue;
                }
                run_last_commands(atoi(num_str));
            }
            else if (is_number(arg1)) {
                char *arg2 = strtok(NULL, " ");
                if (arg2 == NULL) {
                    run_single_command(atoi(arg1));
                }
                else if (is_number(arg2)) {
                    run_command_range(atoi(arg1), atoi(arg2));
                }
                else {
                    printf("Sử dụng: run [num] hoặc run [start] [end] hoặc run y [keyword] hoặc run n [keyword] hoặc run first [num] hoặc run last [num] hoặc run\n");
                }
            }
            else {
                printf("Sử dụng: run [num] hoặc run [start] [end] hoặc run y [keyword] hoặc run n [keyword] hoặc run first [num] hoặc run last [num] hoặc run\n");
            }
        }
        else if (strcmp(token, "clear") == 0) {
            char *arg1 = strtok(NULL, " ");
            if (arg1 == NULL) {
                clear_shell_history();
                continue;
            }
            
            if (strcmp(arg1, "with") == 0 || strcmp(arg1, "but") == 0) {
                int filter_type = (strcmp(arg1, "with") == 0) ? 1 : 0;
                char *keyword = get_remaining_input(arg1 + strlen(arg1) + 1);
                if (keyword == NULL || *keyword == '\0') {
                    printf("Thiếu từ khóa\n");
                    continue;
                }
                delete_filtered_commands(filter_type, keyword);
            }
            else if (strcmp(arg1, "first") == 0) {
                char *num_str = strtok(NULL, " ");
                if (num_str == NULL || !is_number(num_str)) {
                    printf("Thiếu số lượng hoặc số không hợp lệ\n");
                    continue;
                }
                delete_first_commands(atoi(num_str));
            }
            else if (strcmp(arg1, "last") == 0) {
                char *num_str = strtok(NULL, " ");
                if (num_str == NULL || !is_number(num_str)) {
                    printf("Thiếu số lượng hoặc số không hợp lệ\n");
                    continue;
                }
                delete_last_commands(atoi(num_str));
            }
            else if (is_number(arg1)) {
                char *arg2 = strtok(NULL, " ");
                if (arg2 == NULL) {
                    delete_command(atoi(arg1));
                }
                else if (is_number(arg2)) {
                    delete_command_range(atoi(arg1), atoi(arg2));
                }
                else {
                    printf("Sử dụng: clear [num] hoặc clear [start] [end] hoặc clear y [keyword] hoặc clear n [keyword] hoặc clear first [num] hoặc clear last [num] hoặc clear\n");
                }
            }
            else {
                printf("Sử dụng: clear [num] hoặc clear [start] [end] hoặc clear y [keyword] hoặc clear n [keyword] hoặc clear first [num] hoặc clear last [num] hoặc clear\n");
            }
        }
        else if (strcmp(token, "save") == 0) {
            char *arg1 = strtok(NULL, " ");
            if (arg1 == NULL) {
                save_all_commands();
                continue;
            }
            
            if (strcmp(arg1, "with") == 0 || strcmp(arg1, "but") == 0) {
                int filter_type = (strcmp(arg1, "with") == 0) ? 1 : 0;
                char *keyword = get_remaining_input(arg1 + strlen(arg1) + 1);
                if (keyword == NULL || *keyword == '\0') {
                    printf("Thiếu từ khóa\n");
                    continue;
                }
                save_filtered_commands(filter_type, keyword);
            }
            else if (strcmp(arg1, "first") == 0) {
                char *num_str = strtok(NULL, " ");
                if (num_str == NULL || !is_number(num_str)) {
                    printf("Thiếu số lượng hoặc số không hợp lệ\n");
                    continue;
                }
                save_first_commands(atoi(num_str));
            }
            else if (strcmp(arg1, "last") == 0) {
                char *num_str = strtok(NULL, " ");
                if (num_str == NULL || !is_number(num_str)) {
                    printf("Thiếu số lượng hoặc số không hợp lệ\n");
                    continue;
                }
                save_last_commands(atoi(num_str));
            }
            else if (is_number(arg1)) {
                char *arg2 = strtok(NULL, " ");
                if (arg2 == NULL) {
                    save_single_command(atoi(arg1));
                }
                else if (is_number(arg2)) {
                    save_command_range(atoi(arg1), atoi(arg2));
                }
                else {
                    printf("Sử dụng: save [num] hoặc save [start] [end] hoặc save y [keyword] hoặc save n [keyword] hoặc save first [num] hoặc save last [num] hoặc save\n");
                }
            }
            else {
                printf("Sử dụng: save [num] hoặc save [start] [end] hoặc save y [keyword] hoặc save n [keyword] hoặc save first [num] hoặc save last [num] hoặc save\n");
            }
        }
        else if (strcmp(token, "change") == 0) {
            char *arg1 = strtok(NULL, " ");
            if (arg1 == NULL) {
                change_all_commands();
            }
            else if (strcmp(arg1, "with") == 0 || strcmp(arg1, "but") == 0) {
                int filter_type = (strcmp(arg1, "with") == 0) ? 1 : 0;
                char *keyword = get_remaining_input(arg1 + strlen(arg1) + 1);
                if (keyword == NULL || *keyword == '\0') {
                    printf("Thiếu từ khóa\n");
                    continue;
                }
                change_filtered_commands(filter_type, keyword);
            }
            else if (strcmp(arg1, "first") == 0) {
                char *num_str = strtok(NULL, " ");
                if (num_str == NULL || !is_number(num_str)) {
                    printf("Thiếu số lượng hoặc số không hợp lệ\n");
                    continue;
                }
                change_first_commands(atoi(num_str));
            }
            else if (strcmp(arg1, "last") == 0) {
                char *num_str = strtok(NULL, " ");
                if (num_str == NULL || !is_number(num_str)) {
                    printf("Thiếu số lượng hoặc số không hợp lệ\n");
                    continue;
                }
                change_last_commands(atoi(num_str));
            }
            else if (is_number(arg1)) {
                char *arg2 = strtok(NULL, " ");
                if (arg2 == NULL) {
                    change_single_command(atoi(arg1));
                }
                else if (is_number(arg2)) {
                    change_command_range(atoi(arg1), atoi(arg2));
                }
                else {
                    printf("Sử dụng: change [num] hoặc change [start] [end] hoặc change with [keyword] hoặc change but [keyword] hoặc change first [num] hoặc change last [num] hoặc change\n");
                }
            }
            else {
                printf("Sử dụng: change [num] hoặc change [start] [end] hoặc change with [keyword] hoặc change but [keyword] hoặc change first [num] hoặc change last [num] hoặc change\n");
            }
        }
        else if (strcmp(token, "time") == 0) {
            char *arg1 = strtok(NULL, " ");
            if (arg1 == NULL) {
                show_all_timestamps();
                continue;
            }
            
            if (strcmp(arg1, "with") == 0 || strcmp(arg1, "but") == 0) {
                int filter_type = (strcmp(arg1, "with") == 0) ? 1 : 0;
                char *keyword = get_remaining_input(arg1 + strlen(arg1) + 1);
                if (keyword == NULL || *keyword == '\0') {
                    printf("Thiếu từ khóa\n");
                    continue;
                }
                show_filtered_timestamps(filter_type, keyword);
            }
            else if (strcmp(arg1, "first") == 0) {
                char *num_str = strtok(NULL, " ");
                if (num_str == NULL || !is_number(num_str)) {
                    printf("Thiếu số lượng hoặc số không hợp lệ\n");
                    continue;
                }
                show_first_timestamps(atoi(num_str));
            }
            else if (strcmp(arg1, "last") == 0) {
                char *num_str = strtok(NULL, " ");
                if (num_str == NULL || !is_number(num_str)) {
                    printf("Thiếu số lượng hoặc số không hợp lệ\n");
                    continue;
                }
                show_last_timestamps(atoi(num_str));
            }
            else if (is_number(arg1)) {
                char *arg2 = strtok(NULL, " ");
                if (arg2 == NULL) {
                    show_single_timestamp(atoi(arg1));
                }
                else if (is_number(arg2)) {
                    show_timestamp_range(atoi(arg1), atoi(arg2));
                }
                else {
                    printf("Sử dụng: time [num] hoặc time [start] [end] hoặc time with [keyword] hoặc time but [keyword] hoặc time first [num] hoặc time last [num] hoặc time\n");
                }
            }
            else {
                printf("Sử dụng: time [num] hoặc time [start] [end] hoặc time with [keyword] hoặc time but [keyword] hoặc time first [num] hoặc time last [num] hoặc time\n");
            }
        }
        else if (strcmp(token, "quit") == 0) {
            break;
        }
        else {
            printf("Lệnh không hợp lệ. Gõ 'help' để xem hướng dẫn.\n");
        }
    }

    if (cmd) free(cmd);
    return 0;
}