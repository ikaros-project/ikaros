class WebUIWidgetBarGraph extends WebUIWidgetCanvas
{

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

        this.draw(data);
    }



    draw(d)
    {
        try {
            let m = this.parameters['module']
            let s = this.parameters['source']
            this.data = d[m][s][0]  // shold concatenate all rows while drawing instaed
            
            if(!this.data)
                return;
        
            let lineWidth = getComputedStyle(this.canvasElement).getPropertyValue('--line-width');  // These should be used only as default - not here
            let color = getComputedStyle(this.canvasElement).getPropertyValue('--color');
            let fill = getComputedStyle(this.canvasElement).getPropertyValue('--fill');

            let n = this.data.length;
            let bar_width = Math.round((this.width-10) / n);
            let bar_height = this.height;
            let max = 1;
        
            this.canvas.clearRect(0, 0, this.width, this.height);
            this.canvas.beginPath();
            
            for(let i=0; i<n; i++)
            {
                let h = (this.data[i]/max)
                this.canvas.rect(5+i*bar_width, bar_height-bar_height*h, bar_width, bar_height*h);
    //            this.canvas.fillStyle = "none";
    //            this.canvas.fill();
                this.canvas.lineWidth = 1;
                this.canvas.strokeStyle = "red";
            }
            
            this.canvas.stroke();
        }
        catch(err)
        {
        }
    }
};



webui_widgets.add('webui-widget-bar-graph', WebUIWidgetBarGraph);
