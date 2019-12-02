//中序遍历Splay得到的每个点的深度序列严格递增

#define lc c[x][0]
#define rc c[x][1]

ll f[maxn],c[maxn][2],v[maxn],s[maxn],st[maxn],siz[maxn],lm[maxn],la[maxn];
bool r[maxn];

inline bool nroot(int x){//判断节点是否为一个Splay的根
    return c[f[x]][0]==x||c[f[x]][1]==x;
}

inline void pushup(int x){//cc
    s[x]=(s[lc]+s[rc]+v[x])%mod;
    siz[x]=siz[lc]+siz[rc]+1;
}

inline void pushr(int x){
    int t=lc;lc=rc;rc=t;r[x]^=1;
}

inline void pushmul(int x,ll c){
    s[x]=s[x]*c%mod;
    v[x]=v[x]*c%mod;
    lm[x]=lm[x]*c%mod;
    la[x]=la[x]*c%mod;
}

inline void pushadd(int x,ll c){
    s[x]=(s[x]+c*siz[x])%mod;
    v[x]=(v[x]+c)%mod;
    la[x]=(la[x]+c)%mod;
}

inline void pushdown(int x){
    if(lm[x]!=1){
        pushmul(lc,lm[x]);pushmul(rc,lm[x]);
        lm[x]=1;
    }
    if(la[x]){
        pushadd(lc,la[x]);pushadd(rc,la[x]);
        la[x]=0;
    }
    if(r[x]){
        if(lc)pushr(lc);if(rc)pushr(rc);
        r[x]=0;
    }
}

inline void rotat(int x){
    int y=f[x],z=f[y],k=c[y][1]==x,w=c[x][!k];
    if(nroot(y))c[z][c[z][1]==y]=x;
    c[x][!k]=y;c[y][k]=w;
    if(w)f[w]=y;f[y]=x;f[x]=z;
    pushup(y);
}

inline void splay(int x){
    int y=x,z=0;
    st[++z]=y;
    while(nroot(y))st[++z]=y=f[y];
    while(z)pushdown(st[z--]);
    while(nroot(x)){
        y=f[x];z=f[y];
        if(nroot(y))rotat((c[y][0]==x)^(c[z][0]==y)?x:y);
        rotat(x);
    }
    pushup(x);
}

inline void access(int x){
    for(int y=0;x;x=f[y=x]){
        splay(x);rc=y;pushup(x);
    }
}

inline void makeroot(int x){//变成原树的根
    access(x);splay(x);
    pushr(x);
}

int findroot(int x){//找真实的根
    access(x);splay(x);
    while(lc){pushdown(x);x=lc;}
    splay(x);
    return x;
}

inline void split(int x,int y){//提取路径,y为根
    makeroot(x);
    access(y);splay(y);
}

inline void link(int x,int y){//x->y,已连边则不操作
    makeroot(x);
    if(findroot(y)!=x)f[x]=y;
}

inline void cut(int x,int y){//无连边则不操作
    makeroot(x);
    if(findroot(y)==x&&f[y]==x&&!c[y][0]){
        f[y]=c[x][1]=0;
        pushup(x);
    }
}
