#include<stdio.h>
#include<wchar.h>
#include<locale.h>
#include<string.h>

#define UTF8_4B 30           // 11110
#define UTF8_3B 14           // 1110
#define UTF8_2B 6            // 110


// turn off terminal's input buffer
void init_terminal();
void restore_terminal();


// get a line. actually get a char at once, please put in loop and set finsih flag check after func call.
int get_line(char* str, unsigned int max_len_includes_terminator_0, bool& finish_flag);

#ifdef __linux__
    #include<termios.h>
    #include<sys/ioctl.h>

    static termios old_attr = {0};
    static bool got_old_attr = 0;

    void init_terminal(){
        tcgetattr(0, &old_attr);
        termios new_attr = old_attr;
        new_attr.c_lflag &= ~(ICANON | ECHO);
        new_attr.c_lflag |= ISIG;
        tcsetattr(0, TCSANOW, &new_attr);
        got_old_attr = 1;
    }

    void restore_terminal(){
        if(!got_old_attr){
            return;
        }

        tcsetattr(0, TCSANOW, &old_attr);
    }



#else
    #ifdef _WIN32
        #include<windows.h>
        #include<conio.h>

        static HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
        static DWORD oldMode = 0;
        static bool got_oldMode = 0;

        void init_terminal(){
            GetConsoleMode(hStdin, &oldMode);
            DWORD newMode = oldMode;
            newMode &= ~ENABLE_LINE_INPUT;
            newMode &= ~ENABLE_ECHO_INPUT;
            newMode |= ENABLE_PROCESSED_INPUT;
            SetConsoleMode(hStdin ,&newMode);
            got_oldMode = 1;
        }

        void restore_terminal(){
            if(!got_oldMode){
                return;
            }
            SetConsoleMode(hStdin, &oldMode);
        }
    #else
        #error "May be unsupported OS, terminating compile"
    #endif
#endif


int get_line(char* buffer, unsigned int buffer_size, bool& finish_flag){
    static int len = 0;
    char utf8_buffer[4] = {0};

    static unsigned int utf8_char_cnt = 0;
    static char utf8_char_end_index[100] = {0};
    static char utf8_bytes[100] = {0};

    if(finish_flag == 1 || (unsigned int)len > buffer_size - 1){
        write(1, "f ", 2);
        finish_flag = 1;
        return -1;
    }
    
    read(0, &buffer[len], 1);

    // check head of char, proccess multi byte char if any
    if(((unsigned char)buffer[len] >> 3) == UTF8_4B){
        utf8_buffer[0] = buffer[len];
        len++;
        for(int i = 1; i < 4; i++){
            read(0, &buffer[len], 1);
            utf8_buffer[i] = buffer[len];
            len++;
        }
        write(1, utf8_buffer, 4);
        utf8_char_end_index[utf8_char_cnt] = len;
        utf8_bytes[utf8_char_cnt] = 4;
        utf8_char_cnt++;
        return len;
    }
    if(((unsigned char)buffer[len] >> 4) == UTF8_3B){
        utf8_buffer[0] = buffer[len];
        len++;
        for(int i = 1; i < 3; i++){
            read(0, &buffer[len], 1);
            utf8_buffer[i] = buffer[len];
            len++;
        }
        write(1, utf8_buffer, 3);
        utf8_char_end_index[utf8_char_cnt] = len;
        utf8_bytes[utf8_char_cnt] = 3;
        utf8_char_cnt++;
        return len;
    }
    if(((unsigned char)buffer[len] >> 5) == UTF8_2B){
        utf8_buffer[0] = buffer[len];
        len++;
        for(int i = 1; i < 2; i++){
            read(0, &buffer[len], 1);
            utf8_buffer[i] = buffer[len];
            len++;
        }
        write(1, utf8_buffer, 2);
        utf8_char_end_index[utf8_char_cnt] = len;
        utf8_bytes[utf8_char_cnt] = 2;
        utf8_char_cnt++;
        return len;
    }


    // proccess delete
    if(buffer[len] == 8 || buffer[len] == 127){
        // utf8 char delete
        if(utf8_char_cnt > 0){
            if(len == utf8_char_end_index[utf8_char_cnt-1]){
                write(1, "\b\b  \b\b", 7);
                len = len - utf8_bytes[utf8_char_cnt - 1];
                utf8_bytes[utf8_char_cnt] = 0;
                utf8_char_end_index[utf8_char_cnt - 1] = 0;
                utf8_char_cnt--;
                return len;
            } 
        }
        // single byte char delete
        if(len > 0) {
            write(1, "\b \b", 3);
            len -= 1;
        }
        return len;
    } 
    else if(buffer[len] < 32 && buffer[len] != '\n'){
        //not a printable char, discard
        return len;
    }

    
    // echo single byte char
    putc(buffer[len], stdout);
    fflush(stdout);

    // finish a line when press Enter
    if(buffer[len] == '\n' || buffer[len] == '\r'){
        utf8_char_cnt = 0; // reset utf8_cnt
        finish_flag = 1;
        buffer[len] = 0;
        int n = len;
        len = 0;
        return n;
    }

    len += 1;
    return len;
}


int strtonum(const char* str, int len){
    int n = 0;
    for(int i = 0; i<len; i++){
        if(!(str[i] > '0'-1 && str[i] < '9'+1)){
            return -1;
        }
        if(str[i] == 0) break;
        n = n*10 + (str[i] - 48);
    }
    return n;
}


bool is_valid_ipv4(const char* ip) {
    if(ip[15] != 0){
        return false;
    }
    if(!(ip[0] > '0'-1 && ip[0] < '9'+1)){
        return false;
    }

    int seg_val = 0;      // 当前段的数值
    int seg_len = 0;      // 当前段的数字长度
    int dot_cnt = 0;      // 点号数量
    int pos = 0;          // 当前位置

    // 检查字符串长度是否超过15（IPv4最大长度）
    while (ip[pos] != '\0') {
        if (pos > 14) return false;  // 超过最大长度
        
        char c = ip[pos];
        if (c >= '0' && c <= '9') {
            // 前导零检查：当前是0且后面有数字
            if (seg_len == 0 && c == '0' && 
               ip[pos+1] != '.' && ip[pos+1] != 0) {
                return false;
            }
            seg_len++;
        }
        else if (c == '.') {
            // 点号前必须存在数字段
            if (seg_len == 0) return false;
            
            // 段值检查（0-255）
            seg_val = strtonum(&ip[pos-seg_len], seg_len);
            if(seg_val > 255){
                return false;
            }
            dot_cnt++;
            if (dot_cnt > 3) return false;  // 超过3个点
            
            // 重置段状态
            seg_val = 0;
            seg_len = 0;
        }
        else {
            return false;  // 非法字符
        }
        pos++;
    }

    // 最终检查：必须4个数字段且最后一段有效
    return (dot_cnt == 3 && seg_len > 0 && seg_val <= 255);
}