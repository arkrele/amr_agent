clear;
clc;
new=load('Humid_01p_1906_2.dat');
old=load('Humid_01p_1906.dat');
N=16;
figure(1)
semilogy(new(:,1),new(:,N),'--','linewidth',1)
hold on
semilogy(old(:,1),old(:,N),'-')
hold off