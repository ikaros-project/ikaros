class WebUIWidgetKeyPoints extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "PARAMETERS", 'control':'header'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'max', 'default':1, 'type':'float', 'control': 'textedit'},
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
        this.keypoints = [];
        this.timestamps = [];

        this.onclick = function () { alert(this.data) }; // last matrix
    }


    requestData(data_set)
    {
        data_set.add(this.parameters['module']+".KEYPOINTS");
        data_set.add(this.parameters['module']+".TIMESTAMPS");
    }


    draw(count,channels)
    {
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
    }


    update(d)
    {
        if(!d)
            return;
        
        try {
            let m = this.parameters['module'];
            this.keypoints = d[m]["KEYPOINTS"];
            this.timestamps = d[m]["TIMESTAMPS"][0];

            if(!this.keypoints)
                return;

            if(!this.timestamps)
                return;

            let count = this.keypoints.length;
            let channels = this.keypoints[0].length;

            this.draw(count,channels);
        }
        catch(err)
        {
            this.draw();
        }
    }
};


webui_widgets.add('webui-widget-key-points', WebUIWidgetKeyPoints);
