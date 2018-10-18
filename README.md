# bkv-c
Binary key-value tuples protocol, c implementation

## 1 Protocl
bkv = kv + kv + kv + ...  
kv = length + key + value  

## 1.1 Length
`length = length(key + value), vary bytes`  
Length byte uses first bit for stopping bit, indicating whether length ends, 0 stands for ending, 1 stands for continuing. 7 bits are used for actual value    
For example, if length is small than 128, one byte is enough; if length is larger than 256, using multiple bytes, the first high bit of every byte is 1, the first high bit of last byte is 0.
```
2 -> 0x02
666 -> 0x851A
88888888 -> 0xAAB1AC38
```

## 1.2 Key
Key use 1 byte, the first bit stands for key type, 0 stands for number key, 1 stands for strings, thus max string key length is 128

# 2 Example
Pack:
```c
bkv* tb = bkv_new();

u_int8_t value[2] = {2, 3};
bkv_add_by_number_key(tb, 1, &value[0], 2);
bkv_add_by_string_key(tb, "version", &value[0], 2);

char* test = "hello";
bkv_add_by_string_key(tb, "test", (u_int8_t *)test, strlen(test));

buffer* b = bkv_pack(tb);
```

Unpack:
```c
bkv_unpack_result* r = bkv_unpack(b->buf, b->size);
if (r->code != 0) {
    printf("unpack fail");
    return;
}
char* test = bkv_get_string_value_from_string_key(r->bkv, "test");
u_int64_t version = bkv_get_number_value_from_string_key(r->bkv, "version");
```

Example output:
```shell
pack result:[27]: 04010102030A8776657273696F6E02030A847465737468656C6C6F

unpack result code:            0
unpack kv size:                3

key[n]:               1 -> value[2]: 0203
key[s]:         version -> value[2]: 0203
key[s]:            test -> value[5]: 68656C6C6F (s: hello)
```