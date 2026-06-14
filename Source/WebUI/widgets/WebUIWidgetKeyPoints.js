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
            {'name':'channel_mode', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name': "STYLE", 'control':'header'},
            {'name':'color', 'default':"", 'type':'string', 'control': 'textedit'}
        ]
    };


    init()
    {
        super.init();
        this.static_cache = undefined;
        this.drag_selection = undefined;
        this.onclick = function ()
        {
            if(main.edit_mode)
                return;
        }; // last matrix
        this.addEventListener("mousedown", (event) => this.startTimeSelection(event));
    }



// TODO: Flip coordinate system
// TODO: Use ranges
// TODO: Draw current input at current position
// TODO: Draw servo feedback (input) at current position

    draw_recordning()
    {
        this.canvas.fillStyle = '#333';
        this.canvas.fillRect(0, 0, this.width, this.height);
    }



    isRecordMode(channel_mode, channel)
    {
        if(!Array.isArray(channel_mode))
            return false;

        if(Array.isArray(channel_mode[channel]))
            return Number(channel_mode[channel][2]) == 1;

        if(channel_mode.length == 1 && Array.isArray(channel_mode[0]))
            return this.isRecordMode(channel_mode[0], channel);

        if(channel_mode.length >= (channel+1)*4 && Number(channel_mode[channel*4+2]) == 1)
            return true;

        return false;
    }


    mixHash(hash, value)
    {
        let v = Number(value);
        if(Number.isFinite(v))
            v = Math.round(v*1000);
        else
            v = 0x9e3779b9;

        return Math.imul(hash ^ v, 16777619);
    }


    getStaticSignature(sequence, end_time, ranges, channels)
    {
        let hash = 2166136261;
        hash = this.mixHash(hash, this.width);
        hash = this.mixHash(hash, this.height);
        hash = this.mixHash(hash, end_time);
        hash = this.mixHash(hash, sequence.keypoints.length);
        hash = this.mixHash(hash, channels);

        for(let c=0; c<channels; c++)
        {
            let range = ranges[c];
            if(range == undefined)
            {
                hash = this.mixHash(hash, -9999);
                continue;
            }

            hash = this.mixHash(hash, range[0]);
            hash = this.mixHash(hash, range[1]);
        }

        for(let i=0; i<sequence.keypoints.length; i++)
        {
            let keypoint = sequence.keypoints[i];
            hash = this.mixHash(hash, keypoint.time);

            for(let c=0; c<channels; c++)
            {
                let point = keypoint.point[c];
                hash = point == null ? this.mixHash(hash, -8888) : this.mixHash(hash, point);
            }
        }

        return hash;
    }


    addDiamondPath(context, x, y, r)
    {
        context.moveTo(x, y-r);
        context.lineTo(x+r, y);
        context.lineTo(x, y+r);
        context.lineTo(x-r, y);
        context.closePath();
    }


    clamp(value, low, high)
    {
        return Math.max(low, Math.min(high, value));
    }


    eventToFraction(event)
    {
        let rect = this.getBoundingClientRect();
        if(rect.width <= 0)
            return 0;

        return this.clamp((event.clientX-rect.left)/rect.width, 0, 1);
    }


    updateTimeSelection(event, finish=false)
    {
        if(this.drag_selection == undefined)
            return;

        event.preventDefault();
        event.stopPropagation();

        let fraction = this.eventToFraction(event);
        let end_time = this.drag_selection.end_time;
        let time = fraction*end_time;
        let start_time = this.drag_selection.start_time;

        this.drag_selection.current_time = time;
        this.drag_selection.position = fraction;

        this.send_control_change(this.parameters.position, fraction, 0);

        if(Math.abs(time-start_time) > end_time*0.002 || finish)
        {
            let range_start = Math.min(start_time, time);
            let range_end = Math.max(start_time, time);
            this.drag_selection.mark_start = range_start;
            this.drag_selection.mark_end = range_end;
            this.send_command(this.parameters.sequence.substring(0, this.parameters.sequence.lastIndexOf('.')) + ".set_mark_range", 0, range_start, range_end);
        }

        controller.flushCommandQueue();

        if(this.last_draw != undefined)
            this.draw(
                this.last_draw.sequence,
                this.drag_selection.position,
                this.last_draw.start_time,
                this.last_draw.end_time,
                this.drag_selection.mark_start,
                this.drag_selection.mark_end,
                this.last_draw.target,
                this.last_draw.output,
                this.last_draw.input,
                this.last_draw.active,
                this.last_draw.ranges,
                this.last_draw.channel_mode
            );
    }


    finishTimeSelection(event)
    {
        this.updateTimeSelection(event, true);
        this.drag_selection = undefined;

        document.removeEventListener("mousemove", this.drag_move_handler, true);
        document.removeEventListener("mouseup", this.drag_end_handler, true);
        this.drag_move_handler = undefined;
        this.drag_end_handler = undefined;
    }


    startTimeSelection(event)
    {
        if(main.edit_mode || event.button != 0 || this.last_draw == undefined)
            return;

        event.preventDefault();
        event.stopPropagation();

        let fraction = this.eventToFraction(event);
        let end_time = this.last_draw.end_time;
        let time = fraction*end_time;

        this.drag_selection = {
            start_time: time,
            current_time: time,
            mark_start: time,
            mark_end: time,
            position: fraction,
            end_time: end_time
        };

        this.send_control_change(this.parameters.position, fraction, 0);
        controller.flushCommandQueue();

        this.drag_move_handler = (move_event) => this.updateTimeSelection(move_event);
        this.drag_end_handler = (up_event) => this.finishTimeSelection(up_event);
        document.addEventListener("mousemove", this.drag_move_handler, true);
        document.addEventListener("mouseup", this.drag_end_handler, true);

        this.updateTimeSelection(event);
    }


    getStaticCache(sequence, end_time, ranges, channels)
    {
        let signature = this.getStaticSignature(sequence, end_time, ranges, channels);
        if(this.static_cache != undefined && this.static_cache.signature == signature)
            return this.static_cache;

        let n = sequence.keypoints.length;
        let inc = 1;
        while(n > 0 && inc*this.width/n < 10)
            inc *= 2;

        let time_scale = end_time > 0 ? this.width/end_time : 0;
        let screen_points = new Array(channels);

        for(let c=0; c<channels; c++)
        {
            screen_points[c] = [];

            let range = ranges[c];
            if(range == undefined)
                continue;

            let low = range[0];
            let high = range[1];
            let value_scale = high != low ? this.height/(high-low) : 0;

            for(let i=0; i<n; i++)
            {
                let point = sequence.keypoints[i].point[c];
                if(point == null)
                    continue;

                let time = sequence.keypoints[i].time;
                screen_points[c].push({
                    time: time,
                    x: time*time_scale,
                    y: (point-low)*value_scale
                });
            }
        }

        let canvas = document.createElement('canvas');
        canvas.width = this.width;
        canvas.height = this.height;
        let context = canvas.getContext('2d');

        context.strokeStyle = "yellow";
        context.lineWidth = 0.75;
        context.setLineDash([0]);

        for(let c=0; c<channels; c++)
        {
            let points = screen_points[c];
            if(points.length == 0)
                continue;

            context.beginPath();
            for(let i=0; i<points.length; i+=inc)
            {
                let point = points[i];
                if(i == 0)
                {
                    context.beginPath();
                    context.setLineDash([3]);
                    context.moveTo(0, point.y);
                    context.lineTo(point.x, point.y);
                    context.stroke();
                    context.setLineDash([]);
                    context.beginPath();
                    context.moveTo(point.x, point.y);
                }
                else
                    context.lineTo(point.x, point.y);
            }
            context.stroke();

            context.setLineDash([3]);
            let last = points[points.length-1];
            context.beginPath();
            context.moveTo(this.width, last.y);
            context.lineTo(last.x, last.y);
            context.stroke();
            context.setLineDash([]);
        }

        context.beginPath();
        for(let c=0; c<channels; c++)
            for(let point of screen_points[c])
                this.addDiamondPath(context, point.x, point.y, 4);

        context.fillStyle = "yellow";
        context.fill();

        this.static_cache = {
            signature: signature,
            canvas: canvas,
            screen_points: screen_points
        };

        return this.static_cache;
    }


    draw(sequence, f, start_time, end_time, mark_start, mark_end, target, output, input, active, ranges, channel_mode)
    {
        if(ranges==undefined)
            return;

        this.last_draw = { sequence, start_time, end_time, target, output, input, active, ranges, channel_mode };

        let n = sequence.keypoints.length;
        let channels = n != 0 ? sequence.keypoints[0].point.length : 0;
        let selection_start = Math.min(mark_start, mark_end);
        let selection_end = Math.max(mark_start, mark_end);
        let has_selection = mark_start >= 0 && mark_end >= 0 && selection_end > selection_start;
        let record_mode = new Array(channels);

        for(let c=0; c<channels; c++)
            record_mode[c] = this.isRecordMode(channel_mode, c);

        let static_cache = this.getStaticCache(sequence, end_time, ranges, channels);
        let screen_points = static_cache.screen_points;

        this.canvas.fillStyle = '#aaa';
        this.canvas.fillRect(0, 0, this.width, this.height);

        // Draw selection

        try {
            if(end_time > 0 && mark_start >= 0 && mark_end >= mark_start)
            {
                this.canvas.fillStyle = '#888';
                this.canvas.fillRect(mark_start/end_time*this.width, 0, mark_end/end_time*this.width-mark_start/end_time*this.width, this.height);

            }
        }
        catch(err)
        {
            // this.draw();
        }

        this.canvas.drawImage(static_cache.canvas, 0, 0);

        // Highlight selected record-mode keypoint segments

        if(has_selection && end_time > 0)
        {
            this.canvas.strokeStyle = "#ff5555";
            this.canvas.lineWidth = 1.25;
            this.canvas.setLineDash([]);
            this.canvas.beginPath();

            for(let c=0; c<channels;c++)
            {
                if(!record_mode[c])
                    continue;

                let previous = undefined;
                let previous_selected = false;
                for(let point of screen_points[c])
                {
                    let selected = selection_start <= point.time && point.time <= selection_end;
                    if(previous != undefined && previous_selected && selected)
                    {
                        this.canvas.moveTo(previous.x, previous.y);
                        this.canvas.lineTo(point.x, point.y);
                    }

                    previous = point;
                    previous_selected = selected;
                }
            }
            this.canvas.stroke();
        }

        // Draw selected record-mode keypoints

        this.canvas.lineWidth = 0.75;
        this.canvas.setLineDash([]);

        this.canvas.beginPath();
        for(let c=0; c<channels;c++)
        {
            if(!record_mode[c])
                continue;

            for(let point of screen_points[c])
            {
                if(!(selection_start <= point.time && point.time <= selection_end))
                    continue;

                this.addDiamondPath(this.canvas, point.x, point.y, 4);
            }
        }
        this.canvas.fillStyle = "#ff5555";
        this.canvas.fill();


       // draw selection

       if(mark_start != 0)
       {
           this.setColor(1);
           this.canvas.setLineDash([3]);
           this.canvas.beginPath();
           this.canvas.moveTo(mark_start/end_time*this.width, 0);
           this.canvas.lineTo(mark_start/end_time*this.width, this.height);
       }
   
 
        
        // Draw current position

        this.canvas.beginPath();
        this.canvas.setLineDash([]);
        this.canvas.moveTo(f*this.width, 0);
        this.canvas.lineTo(f*this.width, this.height);
        this.canvas.lineWidth = 2;
        this.canvas.strokeStyle = "red";
   
        this.canvas.stroke();

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
        } // draw target


}


/*
    draw_old(sequence, f, start_time, end_time, mark_start, mark_end, target, output, input, active, ranges)
    {
        if(ranges==undefined)
            return;

        this.canvas.fillStyle = '#aaa';
        this.canvas.fillRect(0, 0, this.width, this.height);

        this.canvas.lineStyle = 'gray';
        this.canvas.fillStyle = 'gray';
        this.canvas.lineWidth = 0.25;

        let n = sequence.keypoints.length;
        let inc = 1;
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


            for(let i=n-1; i>=0; i--) // FIXME:
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

        if(n<100)
        {
    
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
        

            // Draw current position

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
    }
*/

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
            let channel_mode = this.getSource("channel_mode", sequence.channel_mode);


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
            let position = f[0];

            if(this.drag_selection != undefined)
            {
                position = this.drag_selection.position;
                start_mark_time = this.drag_selection.mark_start;
                end_mark_time = this.drag_selection.mark_end;
            }

            this.draw(sequence, position, start_time, end_time, start_mark_time, end_mark_time, target, output, input, active, ranges, channel_mode);
        }
        catch(err)
        {
            // this.draw();
        }
    }
};


webui_widgets.add('webui-widget-key-points', WebUIWidgetKeyPoints);
