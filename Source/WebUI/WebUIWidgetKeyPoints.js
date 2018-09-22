class WebUIWidgetKeyPoints extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "PARAMETERS", 'control':'header'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name': "STYLE", 'control':'header'},
            {'name':'color', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'show_title', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};


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


    draw()
    {
        this.canvas.canvas.beginPath();
        this.canvas.canvas.lineWidth = 1;
        this.canvas.canvas.strokeStyle = "gray";

        this.canvas.moveTo(0, this.format.marginTop);
        this.canvas.lineTo(this.width, this.format.marginTop);

        this.canvas.moveTo(0, this.format.height);
        this.canvas.lineTo(this.width, this.format.height);

        this.canvas.moveTo(this.format.marginLeft, 0);
        this.canvas.lineTo(this.format.marginLeft, this.height);

        this.canvas.moveTo(this.width-this.format.marginRight, 0);
        this.canvas.lineTo(this.width-this.format.marginRight, this.height);

        this.canvas.stroke();

    }


    update(d)
    {
        if(!d)
            return;
        
        try {
            let m = this.parameters['module'];
            this.keypoints = d[m]["KEYPOINTS"];
            this.timestamps = d[m]["TIMESTAMPS"];

            if(!this.keypoints)
                return;

            if(!this.timestamps)
                return;

            let size_y = this.keypoints.length;
            let size_x = this.keypoints[0].length;

            this.draw();
        }
        catch(err)
        {
            this.draw();
        }
    }
};


webui_widgets.add('webui-widget-key-points', WebUIWidgetKeyPoints);
