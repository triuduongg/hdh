// Biên dịch chương trình gcc -o gui gui.c `pkg-config --cflags --libs gtk+-3.0`
#include <gtk/gtk.h>
#include <stdlib.h>

GtkWidget *entry;
GtkWidget *text_view;

void on_run_clicked(GtkButton *button, gpointer user_data) {
    const gchar *cmd_input = gtk_entry_get_text(GTK_ENTRY(entry));
    if (!cmd_input || *cmd_input == '\0') return;

    // Nếu người dùng gõ 'quit' thì thoát ứng dụng luôn
    if (g_strcmp0(cmd_input, "quit") == 0) {
        gtk_main_quit();
        return;
    }

    // Gọi chương trình ./h và truyền lệnh qua stdin
    char full_cmd[1024];
    snprintf(full_cmd, sizeof(full_cmd), "echo \"%s\nquit\" | ./h", cmd_input);

    FILE *fp = popen(full_cmd, "r");
    if (!fp) return;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, "", -1); // Clear nội dung cũ

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_insert(buffer, &end, line, -1);
    }

    pclose(fp);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "COMMAND HISTORY MANAGER");
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 5);

    GtkWidget *btn_run = gtk_button_new_with_label("THỰC HIỆN");
    gtk_box_pack_start(GTK_BOX(vbox), btn_run, FALSE, FALSE, 5);
    g_signal_connect(btn_run, "clicked", G_CALLBACK(on_run_clicked), NULL);

    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 5);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
