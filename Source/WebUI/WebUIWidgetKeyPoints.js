class WebUIWidgetKeyPoints extends WebUIWidgetGraph
{
    static template()
    {
        return [
            { 'name': "KEY POINTS", 'control': 'header' },
            { 'name': 'title', 'default': "Key Points", 'type': 'string', 'control': 'textedit' },

            {'name': "PARAMETERS", 'control':'header'},
            {'name':'position', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'mark_start', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'mark_end', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name': "STYLE", 'control':'header'},
            {'name':'color', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'show_title', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]
    };


    init()
    {
        super.init();
        this.onclick = function () { alert(this.data) }; // last matrix
    }

    /*
    requestData(data_set)
    {

        data_set.add(this.parameters['module']+".KEYPOINTS");
        data_set.add(this.parameters['module']+".TIMESTAMPS");

    }
*/

    draw(count,channels)
    {
        /*
        let w = this.width-this.format.marginLeft-this.format.marginRight;
        let h = this.height-this.format.marginTop-this.format.marginBottom;
        let min = this.parameters.min;
        let max = this.parameters.max;
        let scale = h/(max-min);
        
        this.canvas.setTransform(1, 0, 0, 1, -0.5, -0.5);
        this.canvas.clearRect(0, 0, this.width, this.height);
        this.canvas.translate(this.format.marginLeft, this.format.marginTop);

        this.canvas.lineWidth = 1;
        this.canvas.strokeStyle = "gray";
        this.canvas.rect(0, 0, w, h);
        this.canvas.stroke();
        this.canvas.clip();

        let max_time = Math.max(...this.timestamps);

        this.canvas.lineWidth = 3; //this.format.lineWidth;
//        this.canvas.lineCap = this.format.lineCap;
//        this.canvas.lineJoin = this.format.lineJoin;

        for(let k=0; k<channels; k++)
        {
            this.setColor(k);
            this.canvas.beginPath();
            this.canvas.moveTo(0, scale*(this.keypoints[k][0]-min));
            for(let i=1; i<count; i++)
            {
                let x = (w/count)*i;
                this.canvas.lineTo(x, scale*(this.keypoints[i][k]-min));
            }
            for(let i=1; i<count; i++)
            {
           //     this.canvas.arc();
            }
        }
        */
    }


    draw(f,mark_start, mark_end)
    {
        this.canvas.clearRect(0, 0, this.width, this.height);
        this.setColor(0);
        this.canvas.setLineDash([]);
        this.canvas.beginPath();
        this.canvas.moveTo(f*this.width, 0);
        this.canvas.lineTo(f*this.width, this.height);
        this.canvas.closePath();
        this.canvas.stroke();

        if(mark_start != 0)
        {
            this.setColor(1);
            this.canvas.setLineDash([3]);
            this.canvas.beginPath();
            this.canvas.moveTo(mark_start*this.width, 0);
            this.canvas.lineTo(mark_start*this.width, this.height);
            this.canvas.closePath();
            this.canvas.stroke();
        }

        if(mark_end != 0)
        {
            this.setColor(2);
            this.canvas.setLineDash([3]);
            this.canvas.beginPath();
            this.canvas.moveTo(mark_end*this.width, 0);
            this.canvas.lineTo(mark_end*this.width, this.height);
            this.canvas.closePath();
            this.canvas.stroke();
        }

    }

    update(d)
    {
        if(!d)
            return;
        
        try {

            let f = this.getSource("position");
            let mark_start = this.getSource("mark_start");
            let mark_end = this.getSource("mark_end");
            this.draw(f[0],mark_start, mark_end);
        }
        catch(err)
        {
            // this.draw();
        }
    }
};


webui_widgets.add('webui-widget-key-points', WebUIWidgetKeyPoints);
