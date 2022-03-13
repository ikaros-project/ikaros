class WebUIWidgetKeyPoints extends WebUIWidgetGraph
{
    static template()
    {
        return [
            { 'name': "KEY POINTS", 'control': 'header' },
            { 'name': 'title', 'default': "Key Points", 'type': 'string', 'control': 'textedit' },

            {'name': "PARAMETERS", 'control':'header'},
            {'name':'position', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'target', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'output', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'sequence', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'ranges', 'default':"", 'type':'source', 'control': 'textedit'},
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



// TODO: Flip coordinate system
// TODO: Use ranges
// TODO: Draw output at current position
// TODO: Draw servo feedback (input) at current position

    draw(sequence, f, start_time, end_time, mark_start, mark_end, target, output)
    {
        this.canvas.clearRect(0, 0, this.width, this.height);

        let n = sequence.keypoints.length;
        let channels = n != 0 ? sequence.keypoints[0].point.length : 0;

        // Draw keypoint locations

        for(let i=0; i<n; i++)
        {
            let pos = this.width*sequence.keypoints[i].time/end_time;
            this.canvas.setLineDash([]);
            this.canvas.lineWidth = 0.25;
            this.canvas.beginPath();
            this.canvas.moveTo(pos, 0);
            this.canvas.lineTo(pos, this.height);
            this.canvas.closePath();
            this.canvas.stroke();
        }

        // Draw lines
        
        for(let c=0; c<channels;c++)
        {
            this.canvas.lineWidth = 0.25;

            let y = this.height * (sequence.keypoints[0].point[c]/360);
            let pos = this.width*sequence.keypoints[0].time/end_time;
            this.canvas.setLineDash([3]);
            this.canvas.beginPath();
            this.canvas.moveTo(0, y);
            this.canvas.lineTo(pos, y);
            this.canvas.stroke();

            this.canvas.setLineDash([]);
            this.canvas.beginPath();
            this.canvas.moveTo(pos, y);
            for(let i=0; i<n; i++)
            {
                let pos = this.width*sequence.keypoints[i].time/end_time;
                let y = this.height * (sequence.keypoints[i].point[c]/360);
                this.canvas.lineTo(pos, y);
            }
            this.canvas.stroke();

            pos = this.width*sequence.keypoints[n-1].time/end_time;
            y = this.height * (sequence.keypoints[n-1].point[c]/360);
            this.canvas.setLineDash([3]);
            this.canvas.beginPath();
            this.canvas.moveTo(99999999, y);
            this.canvas.lineTo(pos, y);
            this.canvas.stroke();
        }

        // Draw points
    
        for(let c=0; c<channels;c++)
        {
            for(let i=0; i<n; i++)
            {
                let pos = this.width*sequence.keypoints[i].time/end_time;
                this.canvas.setLineDash([]);
                this.canvas.lineWidth = 1.0;
                this.canvas.beginPath();
                let y = this.height * (sequence.keypoints[i].point[c]/360);
                this.canvas.arc(pos, y, 3, 0, 2 * Math.PI, false);
                this.canvas.fill();
            }
        }

        // Draw target


        for(let c=0; c<channels;c++)
        {
            let pos = f*this.width;
            this.canvas.setLineDash([]);
            this.canvas.lineWidth = 1.0;
            this.canvas.beginPath();
            let y = this.height * (target[c]/360);
            this.canvas.arc(pos, y, 7, 0, 2 * Math.PI, false);
            this.canvas.strokeStyle = 'red';
            this.canvas.stroke();
        }


   // Draw output


   for(let c=0; c<channels;c++)
   {
       let pos = f*this.width;
       this.canvas.setLineDash([]);
       this.canvas.lineWidth = 1.0;
       this.canvas.beginPath();
       let y = this.height * (output[c]/360);
       this.canvas.arc(pos, y, 4, 0, 2 * Math.PI, false);
       this.canvas.fillStyle = 'red';
       this.canvas.fill();
   }




        // Draw positions

        this.canvas.lineWidth = 2;
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
            this.canvas.moveTo(mark_start/end_time*this.width, 0);
            this.canvas.lineTo(mark_start/end_time*this.width, this.height);
            this.canvas.closePath();
            this.canvas.stroke();
        }

        if(mark_end != 0)
        {
            this.setColor(2);
            this.canvas.setLineDash([3]);
            this.canvas.beginPath();
            this.canvas.moveTo(mark_end/end_time*this.width, 0);
            this.canvas.lineTo(mark_end/end_time*this.width, this.height);
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
            let target = this.getSource("target");
            let output = this.getSource("output");
            let sequence = this.getSource("sequence");
            this.data = target;
            let start_time = sequence["start_time"];
            let end_time = sequence["end_time"];
            let start_mark_time = sequence["start_mark_time"];
            let end_mark_time = sequence["end_mark_time"];
            this.draw(sequence, f[0], start_time, end_time, start_mark_time, end_mark_time, target[0], output[0]);
        }
        catch(err)
        {
            // this.draw();
        }
    }
};


webui_widgets.add('webui-widget-key-points', WebUIWidgetKeyPoints);
