#include<bits/stdc++.h>
using namespace std;
typedef long long LL;
const int N=234636;

vector<int>G[N];
int a[N],maxx[N];
int f[N],siz[N],ch[N][2],rev[N],st[N];

inline bool nroot(int x){//判断节点是否为一个Splay的根
    return ch[f[x]][0]==x||ch[f[x]][1]==x;
}

inline void pushup(int x){
    siz[x]=siz[ch[x][0]]+siz[ch[x][1]]+1;
    maxx[x]=x;
    if(ch[x][0]){
        if(a[maxx[ch[x][0]]]>a[maxx[x]])
            maxx[x]=maxx[ch[x][0]];
    }
    if(ch[x][1]){
        if(a[maxx[ch[x][1]]]>a[maxx[x]])
            maxx[x]=maxx[ch[x][1]];
    }
}

inline void pushr(int x){
    int t=ch[x][0];ch[x][0]=ch[x][1];ch[x][1]=t;rev[x]^=1;
}

inline void pushdown(int x){
    if(rev[x]){
        if(ch[x][0])pushr(ch[x][0]);if(ch[x][1])pushr(ch[x][1]);
        rev[x]=0;
    }
}

inline void rotat(int x){
    int y=f[x],z=f[y],k=ch[y][1]==x,w=ch[x][!k];
    if(nroot(y))ch[z][ch[z][1]==y]=x;
    ch[x][!k]=y;ch[y][k]=w;
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
        if(nroot(y))rotat((ch[y][0]==x)^(ch[z][0]==y)?x:y);
        rotat(x);
    }
    pushup(x);
}

inline void access(int x){
    for(int y=0;x;x=f[y=x]){
        splay(x);ch[x][1]=y;pushup(x);
    }
}
inline void makeroot(int x){//变成原树的根
    access(x);splay(x);
    pushr(x);
}

int findroot(int x){//找真实的根
    access(x);splay(x);
    while(ch[x][0]){pushdown(x);x=ch[x][0];}
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
    if(findroot(y)==x&&f[y]==x&&!ch[y][0]){
        f[y]=ch[x][1]=0;
        pushup(x);
    }
}

inline void change(int x,int y){
    splay(x);
    a[x]=y;
    pushup(x);
}

inline int ask(int x,int y){
    makeroot(x);
    access(y);splay(y);
    return maxx[y];
}

struct node{
    int u,v,a,b;

    bool operator < (const node &x)const {
        return a<x.a;
    }
}e[N];

int n,m;


int main(){
    int cases=1;
    for(int iii=1;iii<=cases;++iii){
        scanf("%d%d",&n,&m);
        for(int i=0;i<m;++i){
            int x,y,aa,bb;
            scanf("%d%d%d%d",&x,&y,&aa,&bb);
            e[i].u=x;
            e[i].v=y;
            e[i].a=aa;
            e[i].b=bb;
        }
        for(int i=1;i<=n+m+1;++i)
            maxx[i]=i;
        sort(e,e+m);
        int ans=-1;
        for(int i=0;i<m;++i){
            if(e[i].u==e[i].v)continue;
            if(findroot(e[i].u)==findroot(e[i].v)){
                int x=ask(e[i].u,e[i].v);
                if(e[i].b>=a[x])continue;
                int xx=x-n-1;
                cut(e[xx].u,x);
                cut(e[xx].v,x);
            }
            a[i+n+1]=e[i].b;
            link(e[i].u,i+n+1);
            link(e[i].v,i+n+1);

            if(findroot(1)==findroot(n)){
                int temp=ask(1,n);
                temp=a[temp]+e[i].a;
                if(ans==-1)
                    ans=temp;
                else ans=min(ans,temp);
            }
        }
        printf("%d\n",ans);
    }
}
