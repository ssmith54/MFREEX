
clear all
close all

PLOT_GRAPHS = false;
PLOT_DOMAINS_INFLUENCE = true; 


WITH_MOULD = false;
TMAX = 0.6;





path = './../../build/bin/preform_alt/Displacement/';
pathSR = './../../build/bin/preform_alt/srRod/';
boundaryNodes = csvread('./../../build/bin/preform_alt/boundary.txt');
path_base = './../../build/bin/preform_alt';

boundaryNodes = [boundaryNodes;boundaryNodes(1)];

addpath(path)
displacementdir = path ;
d = dir(displacementdir);
d1 = dir([displacementdir,'*.csv']);
numFiles = size(d,1) -3 ;

plotFiles = ceil(linspace(1,numFiles,10));

plot_point =177;
filename = strcat(path,'displacement_',num2str(plotFiles(1)),'.txt');
disp = csvread(filename);

% draw mould
Radius_mould = 24;
Height_mould = 30;
thickness_mould = 5;
mould_length = 180;
height = 74.2650;
Radius_out = 11.25;
top_point = 48.920;



mould_nodes = [];
mould_nodes = [mould_nodes ; [Radius_out,height+5]   ];
mould_nodes = [mould_nodes ; [Radius_out,68.92]   ];
mould_nodes = [mould_nodes ; [Radius_out+Radius_mould,top_point]   ];
mould_nodes = [mould_nodes ; [Radius_out+Radius_mould,height-mould_length]   ];
mould_nodes = [mould_nodes ; [0,height-mould_length]   ];


mould_nodes = [mould_nodes ; [0,height-mould_length-thickness_mould]   ];
mould_nodes = [mould_nodes ; [Radius_out+Radius_mould+thickness_mould,height-mould_length-thickness_mould]   ];
mould_nodes = [mould_nodes ; [Radius_out+Radius_mould+thickness_mould,top_point]   ];
mould_nodes = [mould_nodes ; [Radius_out+thickness_mould,68.92]   ];
mould_nodes = [mould_nodes ; [Radius_out+thickness_mould,height+5]   ];
mould_nodes = [mould_nodes ; [Radius_out,height+5]   ];

if ( WITH_MOULD == false)
    mould_nodes = zeros(1,2);
end




figure



% -------------------------------------------------------------------------%
%                           PLOT 1
% -------------------------------------------------------------------------%



subplot(1,3,1)       % add first plot in 2 x 2 grid

hold on 
fill(mould_nodes(:,1),mould_nodes(:,2),'w');
hold on
fill(-mould_nodes(:,1),mould_nodes(:,2),'w');
hold on
plot(disp(:,1),disp(:,2),'k.','markersize',3)           % line plot
hold on
plot(-disp(:,1),disp(:,2),'k.','markersize',3)           % line plot
ymax = max(disp(:,2));



axis equal 
hold on
%plot(disp(boundaryNodes,1),disp(boundaryNodes,2),'b-')
hold on
%plot(-disp(boundaryNodes,1),disp(boundaryNodes,2),'b-')


boundary_nodes_xy = disp(boundaryNodes,1:2);
Rout = max(disp(:,1));
height = max(disp(:,2));




hold on
filename = strcat(pathSR,'srRod_',num2str(plotFiles(1)),'.csv');
disp = csvread(filename,1);
hold on
plot(disp(:,1),disp(:,2),'r-','linewidth',1)           % line plot
hold on
plot(-disp(:,1),disp(:,2),'r-','linewidth',1)           % line plot
xlim([-50,50])
ylim([-170,ymax])

filename = strcat(path_base,'/MaterialPoints/materialpoints_',num2str(plotFiles(1)),'.txt');
material_points = csvread(filename,1);
filename = strcat(path_base,'/MaterialPoints/Domains/domains_',num2str(plotFiles(1)),'.txt');
domains = csvread(filename);
ellipse_drawer = @(t) draw_ellipse(domains(t,1),domains(t,2),material_points(t,1),material_points(t,2));
ellipse_drawer(570);


% -------------------------------------------------------------------------%
%                           PLOT 2
% -------------------------------------------------------------------------%




filename = strcat(path,'displacement_',num2str(plotFiles(6)),'.txt');
disp = csvread(filename);
subplot(1,3,2)       % add first plot in 2 x 2 grid
hold on 
fill(mould_nodes(:,1),mould_nodes(:,2),'w');
hold on
fill(-mould_nodes(:,1),mould_nodes(:,2),'w');
hold on



plot(disp(:,1),disp(:,2),'k.','markersize',3)           % line plot
hold on
plot(-disp(:,1),disp(:,2),'k.','markersize',3) % line plot
hold on
%plot(disp(plot_point,1),disp(plot_point,2),'r*')
axis equal
hold on
%plot(disp(boundaryNodes,1),disp(boundaryNodes,2),'b-')
hold on
%plot(-disp(boundaryNodes,1),disp(boundaryNodes,2),'b-')
%plot(disp(boundaryNodes,1),disp(boundaryNodes,2),'b-')
hold on
filename = strcat(pathSR,'srRod_',num2str(plotFiles(6)),'.csv');
disp = csvread(filename,1);
hold on
plot(disp(:,1),disp(:,2),'r-','linewidth',1)           % line plot
hold on
plot(-disp(:,1),disp(:,2),'r-','linewidth',1)           % line plot
xlim([-50,50])
ylim([-170,ymax])

filename = strcat(path_base,'/MaterialPoints/materialpoints_',num2str(plotFiles(6)),'.txt');
material_points = csvread(filename,1);
filename = strcat(path_base,'/MaterialPoints/Domains/domains_',num2str(plotFiles(6)),'.txt');
domains = csvread(filename);
ellipse_drawer = @(t) draw_ellipse(domains(t,1),domains(t,2),material_points(t,1),material_points(t,2));

for i = 1:10:length(material_points);
    
   ellipse_drawer(i); 
end



% -------------------------------------------------------------------------%
%                           PLOT 3
% -------------------------------------------------------------------------%
filename = strcat(path,'displacement_',num2str(plotFiles(10)),'.txt');
disp = csvread(filename);




subplot(1,3,3)       % add first plot in 2 x 2 grid

hold on 
fill(mould_nodes(:,1),mould_nodes(:,2),'w');
hold on
fill(-mould_nodes(:,1),mould_nodes(:,2),'w');
hold on

plot(disp(:,1),disp(:,2),'k.','markersize',5)           % line plot
hold on
plot(-disp(:,1),disp(:,2),'k.','markersize',5)           % line plot
hold on
%plot(disp(boundaryNodes,1),disp(boundaryNodes,2),'b-')
hold on
%plot(-disp(boundaryNodes,1),disp(boundaryNodes,2),'b-')
hold on
%plot(disp(plot_point,1),disp(plot_point,2),'r*')

axis equal
hold on
filename = strcat(pathSR,'srRod_',num2str(plotFiles(10)),'.csv');
disp = csvread(filename,1);
hold on
plot(disp(:,1),disp(:,2),'r-','linewidth',1)           % line plot
hold on
plot(-disp(:,1),disp(:,2),'r-','linewidth',1)           % line plot
xlim([-50,50])
ylim([-170,ymax])

filename = strcat(path_base,'/MaterialPoints/materialpoints_',num2str(plotFiles(10)),'.txt');
material_points = csvread(filename,1);
filename = strcat(path_base,'/MaterialPoints/Domains/domains_',num2str(plotFiles(10)),'.txt');
domains = csvread(filename);
ellipse_drawer = @(t) draw_ellipse(domains(t,1),domains(t,2),material_points(t,1),material_points(t,2));

for i = 1:10:length(material_points);
    
   ellipse_drawer(i); 
end


print -dpng2 Displacement.png


% figure
% 
% cells = csvread('./../../build/bin/preform_alt/search_cells.csv');
% active_cells =  csvread('./../../build/bin/preform_alt/active_cells.csv');
% 
% 
% filename = strcat(path,'displacement_',num2str(plotFiles(10)),'.txt');
% disp = csvread(filename);
% 
% 
% plot(disp(:,1),disp(:,2),'k.','markersize',5)           % line plot
% 
% cells = csvread('./../../build/bin/preform_alt/search_cells.csv');
% active_cells =  csvread('./../../build/bin/preform_alt/active_cells.csv');
% 
% 
% 
% for k = 1:length(active_cells)
% hold on
%     i = active_cells(k)+1;
% rectangle('Position',[cells(i,1), cells(i,3), cells(i,2)-cells(i,1), cells(i,4)-cells(i,3)])
% end
% 
% axis equal

m = dlmread('./../../build/bin/preform_alt/pressureTime.txt',' ');
%m(ceil(length(m)/4):end,:) = smoothdata(m(ceil(length(m)/4):end,:))
figure
plot(m(:,1),m(:,2),'k-','linewidth',3);
xlabel('time');
ylabel('Pressure');
xlim([0,TMAX])
ylim([0,1])

b = csvread("../preform/Experimental/N4T100_exp_pressure.csv");
hold on
plot(b(:,1),b(:,2)/1000,'b--')


c = csvread("../preform/Experimental/N4T100_fe_pressure.csv");

hold on
plot(c(:,1),c(:,2)/1000,'r--')

legend('Mesh-free','Experiemntal','FE (Abaqus)')
ylabel('Pressure (MPa)')
xlabel('Time (s)')


%set axis
set(gca, 'FontName', 'cmr12')
% set x tics and y tics
set(gca,...
'Units','normalized',...
'FontWeight','normal',...
'FontSize',14,... % size ofiguref numbers on axis
'FontName','cmr14') % font name
set(gca,'TickLabelInterpreter', 'latex');

% Y label
ylabel({'Pressure (MPa) '},...
'interpreter','latex',...
'FontSize',14,... % font size
'FontName','cmr14')
% X label
xlabel('Time (s)',...
'interpreter','latex',...
'FontWeight','normal',...
'FontSize',14,... % font size
'FontName','cmr14')
% legend

legend({'Meshfree','Experimental','FE (Abaqus )'},... % { 'legend1', 'legend2',...}
'interpreter','latex',...
'FontSize',12,...
'FontName','cmr14',...
'Location','SouthEast')


set(gcf, 'Color', 'w');

print -dpng2 pressure.png



set(gcf, 'Color', 'w');




function [] = draw_ellipse(a,b,x0,y0)

    t = linspace(0,360,50);
    
    
    a = sqrt(a);
    b = sqrt(b);
    x = x0 + a*cosd(t);
    y = y0 + b*sind(t);
    
    hold on
    plot(x,y,'r-');
    
    
    
    


end




