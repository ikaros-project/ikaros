function map(x, low, high)
{
    return (x-low)/(high-low);
}


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
            {'name':'active', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'input', 'default':"", 'type':'source', 'control': 'textedit'},
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
// TODO: Draw current input at current position
// TODO: Draw servo feedback (input) at current position

    draw_recordning()
    {
        this.canvas.fillStyle = 'red';
        this.canvas.fillRect(0, 0, this.width, this.height);
    }

    draw(sequence, f, start_time, end_time, mark_start, mark_end, target, output, input, active, ranges)
    {
        if(ranges==undefined)
            return;

        this.canvas.fillStyle = '#aaa';
        this.canvas.fillRect(0, 0, this.width, this.height);

        this.canvas.lineStyle = 'gray';
        this.canvas.fillStyle = 'gray';
        this.canvas.lineWidth = 0.25;

        let n = sequence.keypoints.length;
        let channels = n != 0 ? sequence.keypoints[0].point.length : 0;

        // Draw keypoint locations

        if(this.width/n>5)
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
            this.canvas.lineWidth = 1;
            this.canvas.strokeStyle = "yellow";
            let first=true;
            for(let i=0; i<n; i++)
            {
                if(sequence.keypoints[i].point[c] != null)
                {
                    if(first)
                    {
                        first = false;

                        let y = this.height * map(sequence.keypoints[i].point[c],ranges[c][0],ranges[c][1]);
                        let pos = this.width*sequence.keypoints[i].time/end_time;

                        this.canvas.setLineDash([3]);
                        this.canvas.beginPath();
                        this.canvas.moveTo(0, y);
                        this.canvas.lineTo(pos, y);
                        this.canvas.stroke();

                        this.canvas.setLineDash([]);
                        this.canvas.beginPath();
                        this.canvas.moveTo(pos, y);
                    }
                    else
                    {
                        let pos = this.width*sequence.keypoints[i].time/end_time;
                        let y = this.height * map(sequence.keypoints[i].point[c],ranges[c][0],ranges[c][1])
                        this.canvas.lineTo(pos, y);
                    }
                }
                this.canvas.stroke();
            }


            for(let i=n-1; i>=0; i--)
            {
                if(sequence.keypoints[i].point[c])
                {
                    let pos = this.width*sequence.keypoints[i].time/end_time;
                    let y = this.height * map(sequence.keypoints[i].point[c],ranges[c][0],ranges[c][1])
                    this.canvas.setLineDash([3]);
                    this.canvas.lineWidth = 1;
                    this.canvas.beginPath();
                    this.canvas.moveTo(99999999, y);
                    this.canvas.lineTo(pos, y);
                    this.canvas.stroke();
                    break;
                }
            }
        }

        // Draw points
    
        this.canvas.fillStyle = "yellow";
        for(let c=0; c<channels;c++)
        {
            for(let i=0; i<n; i++)
            {
                if(sequence.keypoints[i].point[c] != null)
                {
                    let pos = this.width*sequence.keypoints[i].time/end_time;
                    this.canvas.setLineDash([]);
                    //this.canvas.lineWidth = 1.0;
                    this.canvas.beginPath();
                    let y = this.height * map(sequence.keypoints[i].point[c],ranges[c][0],ranges[c][1])
                    this.canvas.arc(pos, y, 3, 0, 2 * Math.PI, false);
                    this.canvas.fill();
            }   }
        }

        // Draw target

        if(target != undefined)
        {
            for(let c=0; c<channels;c++)
            {
                let pos = f*this.width;
                this.canvas.setLineDash([]);
                this.canvas.lineWidth = 5.0;
                this.canvas.beginPath();
                let y = this.height * map(target[0][c],ranges[c][0],ranges[c][1])
                this.canvas.arc(pos, y, 4, 0, 2 * Math.PI, false);
                this.canvas.moveTo(pos-8, y);
                this.canvas.lineTo(pos+8, y);
                this.canvas.fillStyle = 'black';
                this.canvas.fill();
            }
        }

   // Draw output

   if(output != undefined)
   {
        for(let c=0; c<channels;c++)
        {
            let pos = f*this.width;
            this.canvas.setLineDash([]);
            this.canvas.lineWidth = 1.0;
            this.canvas.beginPath();
            let y = this.height * map(output[0][c],ranges[c][0],ranges[c][1])
            this.canvas.moveTo(pos+15, y);
            this.canvas.lineTo(pos+15-8, y+4);
            this.canvas.lineTo(pos+15-8, y-4);
            this.canvas.lineTo(pos+15, y);
            this.canvas.fillStyle = 'black';
            this.canvas.fill();
        }
   }

      // Draw input

   if(input != undefined)
   {
        for(let c=0; c<channels;c++)
        {
            let pos = f*this.width;
            this.canvas.setLineDash([]);
            this.canvas.lineWidth = 1.0;
            this.canvas.beginPath();
            let y = this.height * map(input[0][c],ranges[c][0],ranges[c][1])
            // this.canvas.arc(pos, y, 9, 0, 2 * Math.PI, false);
            this.canvas.moveTo(pos-5, y);
            this.canvas.lineTo(pos-5-8, y+4);
            this.canvas.lineTo(pos-5-8, y-4);
            this.canvas.lineTo(pos-5, y);
            this.canvas.fillStyle = 'white';
            this.canvas.fill();
        }
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
            let input = this.getSource("input");
            let active = this.getSource("active");
            let sequence = this.getSource("sequence");
            let ranges = this.getSource("ranges");

            if(Object.keys(sequence).length == 0) // RECORDING
            {
                this.draw_recordning();
                return;
            }
            this.data = target;
            let start_time = sequence["start_time"];
            let end_time = sequence["end_time"];
            let start_mark_time = sequence["start_mark_time"];
            let end_mark_time = sequence["end_mark_time"];
            this.draw(sequence, f[0], start_time, end_time, start_mark_time, end_mark_time, target, output, input, active, ranges);
        }
        catch(err)
        {
            // this.draw();
        }
    }
};


webui_widgets.add('webui-widget-key-points', WebUIWidgetKeyPoints);
