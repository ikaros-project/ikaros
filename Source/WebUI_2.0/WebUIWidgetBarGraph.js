class WebUIWidgetBarGraph extends WebUIWidgetCanvas
{
/*
    constructor()
    {
        super();
        this.test = 42;
        console.log("Hej Hej", this.drawLayout);
    }
*/


    static template()
    {
        return [
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};



    init()
    {
        this.data = [10, 83, 32, 56, 2, 7, 55];   // Should connect to main data structure
        
        this.onclick = function () { alert(this.data) };
    }



    requestData(data_set)
    {
        data_set.add(this.parameters['module']+"."+this.parameters['source']);
    }



    update(data)
    {
        let width = parseInt(getComputedStyle(this.canvasElement).width);
        let height = parseInt(getComputedStyle(this.canvasElement).height);
        if(width != this.canvasElement.width || height != this.canvasElement.height)
        {
            this.canvasElement.width = parseInt(width);
            this.canvasElement.height = parseInt(height);
        }
        
        this.width = parseInt(width);
        this.height = parseInt(height);
        
        this.format.width = this.width - this.format.marginLeft - this.format.marginRight;
        this.format.height = this.height - this.format.marginTop - this.format.marginBottom;


        this.draw(data);
    }



    draw(d)
    {
        this.canvas.setTransform(1, 0, 0, 1, 0.5, 0.5);
        this.canvas.clearRect(0, 0, this.width, this.height);
        this.drawTitle();

        this.drawLayout();

        try {
            let m = this.parameters['module']
            let s = this.parameters['source']
            this.data = d[m][s]
            
            if(!this.data)
                return;

            let size_y = this.data.length;
            let size_x = this.data[0].length;
            let pane_y = (this.format.height+1)/size_y;
            
            this.canvas.setTransform(1, 0, 0, 1, this.format.marginLeft+0.5, this.format.marginTop+0.5);
            
            for(let y=0; y<size_y; y++)
            {
                this.canvas.beginPath();
                this.canvas.strokeStyle = "red";
                this.canvas.rect(-1, pane_y*y-1, this.format.width+1, pane_y);
                this.canvas.stroke();

                let n = size_x;
                let bar_width = Math.round(this.format.width/ n);
                let bar_height = pane_y;
                let max = 1;
            
                for(let i=0; i<n; i++)
                {
                    let h = (this.data[y][i]/max)
                    this.canvas.beginPath();
                    this.setColor(i)
                    this.canvas.rect(i*bar_width, pane_y*(1+y)-1 - bar_height*h, bar_width, bar_height*h);
                    this.canvas.fill();
                    this.canvas.stroke();
                }
            }
            
            return;
            
            let n = this.data.length;
            let bar_width = Math.round(this.format.width/ n);
            let bar_height = this.format.height;
            let max = 1;
        
            for(let i=0; i<n; i++)
            {
                let h = (this.data[i]/max)
                this.canvas.beginPath();
                this.setColor(i)
                this.canvas.rect(i*bar_width, bar_height-bar_height*h, bar_width, bar_height*h);
                this.canvas.fill();
                this.canvas.stroke();
            }
        }
        catch(err)
        {
        }
    }
};


//test = new WebUIWidgetBarGraph();

webui_widgets.add('webui-widget-bar-graph', WebUIWidgetBarGraph);
