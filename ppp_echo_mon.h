#define GROUP "FF02:0:0:0:0:0:0:114"
#define SPORT 204
#define DPORT 1986

#define xerror(msg) do { perror(msg); exit(1); } while(0)
#define cerror(msg, expr) do { if (expr) xerror(msg); } while(0)
