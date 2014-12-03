inputfile=input('Input the inputfile:');
data=load(inputfile);
resultfile=input('Input the file storing clustering reslut:');
res=load(resultfile);
index=unique(res(:,2));
halo=res(:,3);
colors='rgbcmyrgbcmyrgbcmyrgbcmyrgbcmyrgbcmyrgbcmyrgbcmyrgbcmy';
symbols='.ox+*s^dv<>ph.ox+*s^dv<>ph.ox+*s^dv<>ph.ox+*s^dv<>ph';
nClus=length(index);
figure;
for i=1:nClus
    idx=find(res(:,2)==index(i));
    nonhaloidx=find(halo(idx)==0);
    if ~isempty(nonhaloidx)
        plot(data(idx(nonhaloidx),1),data(idx(nonhaloidx),2),[symbols(i),colors(i)],'MarkerFaceColor',colors(i),'MarkerSize',5);
        hold on;
    end
     hold on;
end

legend_string=cell(1,nClus);
for i=1:nClus
    legend_string{i}=['cluster ',num2str(i)];
end
legend(legend_string);

haloidx=find(halo==1);
if ~isempty(haloidx)
    plot(data(haloidx,1),data(haloidx,2),['o','k'],'MarkerFaceColor','k','MarkerSize',3);
    hold on;
end

title('Clustering Result');