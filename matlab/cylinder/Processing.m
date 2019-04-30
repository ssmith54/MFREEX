clear all

close all 
path = './../../build/bin/cylinder/Displacement';
path_base = './../../build/bin/cylinder/';
addpath(path)
displacementdir = path ;
d = dir(displacementdir);
d1 = dir([displacementdir,'*.txt']);
numFiles = size(d,1) -3 ;

plotFiles = ceil(linspace(1,numFiles,10));



figure


%material_points = csvread('./../../build/bin/cylinder/Domains/domainmaterialpoints.csv');


filename = strcat(path_base,'/MaterialPoints/materialpoints_',num2str(plotFiles(10)),'.txt');
material_points = csvread(filename,1);





filename = strcat(path,'/displacement_',num2str(plotFiles(10)),'.txt');
disp = csvread(filename);
plot(disp(:,1),disp(:,2),'k.')           % line plot
axis equal
hold on
%plot(material_points(:,1),material_points(:,2),'r.');
xlim([0,40])
ylim([0,30])

filename = strcat(path_base,'/MaterialPoints/Domains/domains_',num2str(plotFiles(10)),'.txt');
domains = csvread(filename);
ellipse_drawer = @(t) draw_ellipse(domains(t,1),domains(t,2),material_points(t,1),material_points(t,2));





hold on
ellipse_drawer(1);

hold on
ellipse_drawer(200);


filename = strcat(path_base,'/MaterialPoints/materialpoints_',num2str(plotFiles(1)),'.txt');
material_points = csvread(filename,1);


filename = strcat(path,'/displacement_',num2str(plotFiles(1)),'.txt');
disp = csvread(filename);
plot(disp(:,1),disp(:,2),'k.')           % line plot
axis equal
hold on
plot(material_points(:,1),material_points(:,2),'b*');
xlim([0,40])
ylim([0,30])

filename = strcat(path_base,'/MaterialPoints/Domains/domains_',num2str(plotFiles(1)),'.txt');
domains = csvread(filename);
hold on
draw_ellipse(domains(1,1),domains(1,2),material_points(1,1),material_points(1,2));
hold on
ellipse_drawer = @(t) draw_ellipse(domains(t,1),domains(t,2),material_points(t,1),material_points(t,2));

ellipse_drawer(200);
saveas(gcf,'Displacement_cylinder','epsc')



sizeA = [2 inf];
formatSpec = '%f %f';
fileID = fopen('./../../build/bin/cylinder/loadDisp.txt','r');
A = fscanf(fileID,formatSpec,sizeA);

A = A';

figure



plot(A(1:10:end,1),A(1:10:end,2),'kx','markersize',6);
% 
sizeA = [2 inf];
formatSpec = '%f %f';
fileID = fopen('./exactSol.txt','r');

B = fscanf(fileID,formatSpec,sizeA);

B = B';

hold on 
plot(B(:,1),B(:,2)/1000,'r-');
xlabel('dr')
ylabel('Pressure')
legend('Meshfree','Exact','Location','northwest');

saveas(gcf,'Solution_cylinder','epsc')


function [] = draw_ellipse(a,b,x0,y0)

    t = linspace(0,360,50);
    
    
    a = sqrt(a);
    b = sqrt(b);
    x = x0 + a*cosd(t);
    y = y0 + b*sind(t);
    
    
    plot(x,y,'r-');
    
    
    
    


end




