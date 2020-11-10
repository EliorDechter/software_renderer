
u8 *load_file_to_buffer(u8 *file_name) {
    FILE *file = fopen(file_name, "r");
    if (!file) {
        pritnf("%s not found/n", file_name);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *buffer = malloc(size); //TODO: how to use a variable sized allocation inside the perm allocator
    fread(buffer, size, 1, file);
    
    fclose(file);
    
    return buffer;
}

typedef struct Tokenizer {
    char *file_name;
    s32 column_number;
    s32 line_number;
    //stream
    
    //char *input;
    //char at[2];
    u8 *current_char;
    
    bool error;
} Tokenizer;

typedef enum Token_type {
    token_type_unkown,
    
    token_type_open_brace,
    token_type_closed_brace,
    token_type_number,
    token_type_string,
    token_type_identifier,
    
    token_type_spacing,
    token_type_eof,
    token_type_comment,
    
    //token_type_end_of_stream
} Token_type;

typedef struct Token {
    string file_name;
    s32 column_number;
    s32 line_number;
    
    Token_type type;
    string text;
    f32 float_value;
    s32 int_value;
} Token;

Token get_raw_token(Tokenizer *tokenizer) {
    Token token = {0};
    token.file_name = tokenizer->file_name;
    token.column_number = tokenizer->column_number;
    token.line_number = tokenizer->line_number;
    token.text = tokenizer->input;
    
    char c = tokenizer->at[0];
    adva
}

u8 move_tokenizer_to_next_char(Tokenizer *tokenizer) {
    //TODO: fix once we know the proper line and column
    tokenizer->column_number ++;
    tokenizer->line_number++;
    
    
}

Token get_next_token(Tokenizer *tokenizer) {
    Token token = {0};
    
    u8 current_char = *tokeinzer->current_char;
    switch(current_char) {
        case '\0': { token.type = token_type_end_of_buffer; break;}
        
        case '{': { token.type = token_type_open_brace; break;}
        case '}': { token.type = token_type_closed_brace; break;}
        case '=': { token.type = token_type_equal; break;}
        case ',': { token.type = token_type_comma; break;}
        case 'identifier': { token.type = token_type_identifier; break;}
    }
    if (current_char == '+' || current_char == '-') {
        u8 sign = current_char;
        ++tokenizer->current_char;
        current_char = *tokenizer->current_char;
        if (!(current_char >= 0 && current_char <= 9)) {
            printf("missing a number after sign at (row: %d column: %d)", tokenizer->row_number, tokenizer->column_number);
        }
        
        String string_number;
        for (int i = 0; current_char >= 0 && current_char <= 9; ++i) {
            assert(i < 10);
            char_num = move_tokenizer_to_next_char(tokenizer);
        }
        
        for (int i = string.size; i < 10; ++i) {
            
        }
    }
}
}

void refill_tokenizer(Tokenizer *tokenizer) {
    //TODO
}

Tokenizer get_tokenizer(Buffer buffer, String string) {
    Tokenizer tokenizer = {
        .file_name = file_name,
        .column_number = 1,
        .line_number = 1,
        .current_char = buffer.data,
    };
    
    return tokenizer;
}

void parse_test_vertices_file() {
    u8 *buffer = load_file_to_buffer("test_vertices.pav");
    
    
    
}