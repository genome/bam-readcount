#bam data
a = read.table("summary.bam.tsv",header=T,sep="\t",stringsAsFactors=F)
a$log=log10(a$num_sites)
a$depth = gsub("x","",a$depth)
afold=sort(as.numeric(unique(a$depth)))
#cram data
b = read.table("summary.cram.tsv",header=T,sep="\t",stringsAsFactors=F)
b$log=log10(b$num_sites)
b$depth = gsub("x","",b$depth)
bfold=sort(as.numeric(unique(b$depth)))

doplot <- function(a,fold){
    col=c('#a6cee3','#1f78b4','#b2df8a','#33a02c','#fb9a99','#e31a1c','#fdbf6f','#ff7f00','#cab2d6','#6a3d9a')
    plot(-100,-100,ylim=c(1,5),xlim=c(2,6),xaxt="n",yaxt="n",xlab="genomic positions",ylab="seconds");
    axis(1,at=seq(1,6),labels=c(10,100,1000,10000,100000,1000000),cex.axis=0.8);
    axis(2,at=seq(1,5),labels=c(10,100,1000,10000,100000),las=2,cex.axis=0.8);
    grid(col="grey50");

    legend(2,5,fill=rev(col),legend=paste(rev(fold),"x",sep=""),cex=0.7,bg="white")

    doplotlog <- function(i,a,col){points(a[a$depth==i,]$log,log10(a[a$depth==i,]$timing_seconds),type="o",pch=18,col=col)}

    count=1;for(i in fold){doplotlog(i,a,col[count]);count=count+1}
}

pdf("figure.pdf",height=4,width=10)
par(mfrow=c(1,2));
doplot(a,afold)
doplot(b,bfold)
dev.off()
