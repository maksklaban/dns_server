struct dns_header {
    unsigned short id; // 16 bit identifier assigned
    unsigned char rd :1; // recursion desired
    unsigned char tc :1; // truncated message
    unsigned char aa :1; // authoritive answer
    unsigned char opcode :4; // specifies kind of query
    unsigned char qr :1; // query | response flag
    unsigned char rcode :4; // response code
    unsigned char cd :1; // checking disabled
    unsigned char ad :1; // authenticated data
    unsigned char z :1; // Reserved for future use
    unsigned char ra :1; // recursion available
    unsigned short qdcount; // number of question entries
    unsigned short ancount; // number of answer records
    unsigned short nscount; // number of authority records
    unsigned short arcount; // number of resource records

};

struct question {
    unsigned short qtype;
    unsigned short qclass;
};

struct rdata{
    unsigned short type;
    unsigned short rclass;
    unsigned int ttl;
    unsigned short rdlength;
};

struct res_record {
    unsigned char *name;
    struct R_DATA *resource;
    unsigned char *rdata;
};
 
typedef struct {
    unsigned char *name;
    struct QUESTION *ques;
} query;
 