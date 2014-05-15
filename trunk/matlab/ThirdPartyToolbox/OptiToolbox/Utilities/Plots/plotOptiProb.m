function plotOptiProb(prob,opts,xb,scale,dolog,npts,mode)
%PLOTOPTIPROB Plot opti problem optimization surface with constraints
%
%   plotOptiProb(prob,xb,scale) plots a contour plot of the optimization
%   surface where prob is an optiprob structure. xb is the solution vector, 
%   and scale zooms the plot.

%   Copyright (C) 2011 Jonathan Currie (I2C2)

%Defaults
if(nargin < 7 || isempty(mode)), mode = 'normal'; end
if(nargin < 6), npts = []; end
if(nargin < 5 || isempty(dolog)), dolog = false; end
if(nargin < 4 || isempty(scale)), scale = 5; end
if(nargin < 3), xb = []; end
if(nargin < 2), error('This function requires two inputs (prob, opts)'); end

ptypes = {'LP','BILP','MILP','QP','QCQP','MIQP','MIQCQP','SDP','MISDP','NLS','DNLS','UNO','NLP','MINLP'};

%Check we have a 'plottable' problem
if(~any(strcmpi(prob.type,ptypes)))
    error('A Problem of type ''%s'' cannot be plotted!',prob.type);
end

%Check for SD problem in SeDuMi format
if(any(strcmpi(prob.type,{'SDP','MISDP'})))
   if(isstruct(prob.sdcone))
       prob = sedumi2opti(prob);
       prob.objective = @(x) prob.f'*x;
   end
end

%Clear existing figure
clf;

%Check if unsolved
if(isempty(xb) && ~strcmpi(mode,'multi'))
    %Check if we have valid x0
    if(~isempty(prob.x0) && ~all(isnan(prob.x0)))
        mode = 'usex0';
        xb = prob.x0;
    elseif(~isempty(prob.lb) && ~isempty(prob.ub) && ~all(isinf(prob.lb)) && ~all(isinf(prob.ub)))
        mode = 'bounded';
        %fake xb for >2D plotting purposes
        xb = (prob.ub-prob.lb)./2 + prob.lb;
        %if user has supplied scale as vector, substitute here
        if(length(scale) > 1)
            mode = 'bounded_scale';
        end
    else
        error('OPTI can only plot unsolved problems that have a specified x0, or if all variables are bounded.');
    end
end

%Only plot fit if normal plot
if(any(strcmpi(mode,{'normal','usex0'})))
    %Check for parameter estimation problem
    if(strcmpi(prob.type,'DNLS'))
        plotDNLS(prob,opts,xb);
        return;
    elseif(strcmpi(prob.type,'NLS'))
        plotDataFit(prob,xb,dolog);
        return;
    end
end

%Setup Plotting Data
data.ndec = prob.sizes.ndec;
if(dolog ~= 1 && dolog ~= 0)
    error('doLog must be 0 or 1!');
end
data.dolog = dolog;
%Error check scale input
if(length(scale) > 1)
    if(length(scale) ~= data.ndec*2)
        error('Expected a %d element vector for specifying the plot boundaries, of the form [x1min x1max x2min x2max ... xNmin xNmax]',data.ndec*2);
    end    
end
data.scale = scale;
%Check number of points
if(~isempty(npts) && (npts < 0 || npts > 3000))
    error('npts must be 0 < n < 3000');
end
data.npts = npts;
data.mode = mode;
data.fixval = xb;
data.idx = 1:data.ndec;
k = 1; n = 1;
%Check for higher dim plot
if(data.ndec > 2)
    for i = 1:data.ndec
        for j = i+1:data.ndec
            subplot(data.ndec-1,data.ndec-1,k);
            data.idx = [i j];
            if(length(scale) > 1) %grab required elements
                data.scale = [scale(i*2-1:i*2) scale(j*2-1:j*2)];
            end
            plotProblem(prob,xb,data);
            title(makeTitle(data,xb)); 
            k = k + 1;
            n = n + 1;
        end
        k = k + i;
    end  
    %Plot Minimum on Figure Title
    if(~isempty(xb))
        data.fval = prob.objective(xb);
    else
        data.fval = [];
    end
    set(gcf,'Name',plotTitle(prob,xb,data));
else
    %Standard single plot
    plotProblem(prob,xb,data);
    set(gcf,'Name','');
end



function plotProblem(prob,xb,data)    
%Plot Objective Contour
data.npts = plotObj(prob,xb,data);

%If we have search space (i.e. multisolve has been run), also plot
if(strcmp(data.mode,'multi') && isfield(prob,'multi') && ~isempty(prob.multi))
    plotMultiSearch(prob,data);
end

%Plot Bounds
if(~strcmpi(prob.type,'bilp'))
    plotBounds(prob.lb,prob.ub,data);
end

%Plot Linear Constraints
if(prob.sizes.nineq + prob.sizes.neq > 0)
    if(data.ndec <= 2)
        %Standard linear patches
        if(~isempty(prob.rl))
            [A,b,Aeq,beq] = row2gen(prob.A,prob.rl,prob.ru);
            plotLinCon(A,b,Aeq,beq);
        else
            plotLinCon(prob.A,prob.b,prob.Aeq,prob.beq);
        end
    else
        %Lazy nonlinear contour method (can't quite work out above for multi-dim)
        pcopy = prob; pcopy.nlcon = []; pcopy.nlrhs = []; pcopy.nle = []; pcopy.cl = []; pcopy.cu = []; 
        if(~isempty(prob.rl))
            pcopy = rowlin2nl(pcopy);
        else
            pcopy = genlin2nl(pcopy);
        end
        %Plot patches by telling algorithm constraints are linear
        plotNonlinCon(pcopy,data,true);
    end
end

%Plot Quadratic Constraints
if(prob.sizes.nqc)
    plotQuadCon(prob.Q,prob.l,prob.qrl,prob.qru,data);
end

%Plot Semidefinite Constraints
if(prob.sizes.nsdcone)
    plotSDCon(prob.sdcone,data);
end

%Plot Nonlinear Constraints
if(~isempty(prob.nle) || ~isempty(prob.cl))
    plotNonlinCon(prob,data);
end

%Check for Integer Constraints
if(any(prob.int.ind))
    plotIntCon(prob,data);
end

%Plot Optimum
if(~isempty(xb))
    idx = data.idx;
    hold on
    switch(data.mode)
        case {'normal','multi'}
            if(length(xb)==1)
                plot(xb(idx(1)),prob.objective(xb),'r.','markersize',20);
            else
                plot(xb(idx(1)),xb(idx(2)),'r.','markersize',20);
            end
        case 'usex0'
            if(length(xb)==1)
                plot(xb(idx(1)),prob.objective(xb),'b.','markersize',20);
            else
                plot(xb(idx(1)),xb(idx(2)),'b.','markersize',20);
            end
    end
    hold off
end

function str = makeTitle(data,xb)
%Construct multi-dim plot title

xs = true(data.ndec,1);
xs(data.idx) = false;
fi = find(xs);
if(length(fi) > 1)
    str = '[';
else
    str = [];
end
for i = 1:length(fi)
    str = sprintf('%sx%d, ',str,fi(i));
end
str = str(1:end-2);
if(~isempty(xb))
    if(length(fi) > 1)
        str = [str '] = ['];
    else
        str = [str ' = '];
    end
    for i = 1:length(fi)
        str = sprintf('%s%g, ',str,xb(fi(i)));
    end
    str = str(1:end-2);
    if(length(fi) > 1)
         str = [str ']'];
    end
end


