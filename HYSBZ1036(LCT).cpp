#include<bits/stdc++.h>
using namespace std;
typedef long long LL;
const int N=34636;

vector<int>G[N];
int n,m;
int a[N],tmax[N],tsum[N];
int f[N],siz[N],ch[N][2],rev[N],st[N];

inline bool nroot(int x){//判断节点是否为一个Splay的根
    return ch[f[x]][0]==x||ch[f[x]][1]==x;
}

inline void pushup(int x){
    siz[x]=siz[ch[x][0]]+siz[ch[x][1]]+1;
    tmax[x]=tsum[x]=a[x];
    if(ch[x][0]){
        tmax[x]=max(tmax[x],tmax[ch[x][0]]);
        tsum[x]+=tsum[ch[x][0]];
    }
    if(ch[x][1]){
        tmax[x]=max(tmax[x],tmax[ch[x][1]]);
        tsum[x]+=tsum[ch[x][1]];
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

inline int querysum(int x,int y){
    makeroot(x);
    access(y);splay(y);
    return tsum[y];
}

inline int querymax(int x,int y){
    makeroot(x);
    access(y);splay(y);
    return tmax[y];
}

void dfs(int x){
    tmax[x]=tsum[x]=a[x];
    siz[x]=1;
    for(int i=0;i<G[x].size();++i){
        int y=G[x][i];
        if(y==f[x])continue;
        f[y]=x;
        dfs(y);
    }
}

int main(){
    int cases=1;
    for(int iii=1;iii<=cases;++iii){
        scanf("%d",&n);
        for(int i=1;i<n;++i){
            int x,y;
            scanf("%d%d",&x,&y);
            G[x].push_back(y);
            G[y].push_back(x);
        }
        for(int i=1;i<=n;++i)
            scanf("%d",&a[i]);

        dfs(1);
        int q;
        scanf("%d",&q);
        for(int qq=0;qq<q;++qq){
            char s[7];
            int x,y;
            scanf("%s",s);
            scanf("%d%d",&x,&y);
            if(s[0]=='Q'){
                if(s[1]=='S')
                    printf("%d\n",querysum(x,y));
                else
                    printf("%d\n",querymax(x,y));
            }
            else{
                change(x,y);
            }
        }

    }
}
