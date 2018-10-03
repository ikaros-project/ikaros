class WebUIWidgetEpiHead extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "PARAMETERS", 'control':'header'},
            {'name':'gazeModule', 'default':"", 'type':'module', 'control': 'textedit'},
            {'name':'gazeSource', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'pupilModule', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'pupilSource', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'eyeRed', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'eyeGreen', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'eyeBlue', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'mouthRed', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'mouthGreen', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'mouthBlue', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'gaze', 'default':0, 'type':'float', 'control': 'slider', 'min': -1.57, 'max': 1.57},
            {'name':'vergence', 'default':0, 'type':'float', 'control': 'slider', 'min': -1.57, 'max': 1.57},
            {'name':'pupil', 'default':0.5, 'type':'float', 'control': 'slider', 'min': 0, 'max': 1},
            {'name': "STYLE", 'control':'header'},
            {'name':'color', 'default':"black", 'type':'string', 'control': 'textedit'},
            {'name':'fill', 'default':"white", 'type':'string', 'control': 'textedit'},
            {'name':'earColor', 'default':"#0088ff", 'type':'string', 'control': 'textedit'},
            {'name':'eyeColor', 'default':"#88aaff", 'type':'string', 'control': 'textedit'},
            {'name':'mouthColor', 'default':"#000000", 'type':'string', 'control': 'textedit'},
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


    requestData(data_set)
    {
        if(this.parameters['gazeModule'] && this.parameters['gazeSource'])
            data_set.add(this.parameters['gazeModule']+"."+this.parameters['gazeSource']);
        if(this.parameters['pupilModule'] && this.parameters['pupilSource'])
            data_set.add(this.parameters['pupilModule']+"."+this.parameters['pupilSource']);
    }


    drawEye(x, y, gaze, pupil)
    {
        this.canvas.save();

        // Eye outline

        this.canvas.translate(x, y);
        this.setColor(0);
        this.canvas.fillStyle = "white";
        this.canvas.beginPath();
        this.canvas.moveTo(37.5, 0);
        this.canvas.quadraticCurveTo(52.5, 0, 52.5, 15);
        this.canvas.lineTo(52.5, 25);
        this.canvas.quadraticCurveTo(52.5, 40, 37.5, 40);
        this.canvas.lineTo(16, 40);
        this.canvas.quadraticCurveTo(0, 40, 0, 25);
        this.canvas.lineTo(0, 15);
        this.canvas.quadraticCurveTo(0, 0, 15, 0);
        this.canvas.closePath();
        this.canvas.fill();
        this.canvas.clip();
        
        let v = 27.5*Math.sin(gaze)
        
        // Iris
        
        this.canvas.fillStyle = this.parameters.eyeColor; // when LED input not connected
        this.canvas.strokeStyle = "#00000000";
        this.canvas.beginPath();
        this.canvas.arc(27.5+v, 20, 17.5, 0, 2*Math.PI);
        this.canvas.fill();

        // Pupil

        this.canvas.fillStyle = "black";
        this.canvas.beginPath();
        this.canvas.arc(27.5+v, 20, pupil*17.5, 0, 2*Math.PI);
        this.canvas.fill();

        this.canvas.restore();
        
        // Draw outline again
        
        this.canvas.save();
        this.canvas.translate(x, y);
        this.setColor(0);
        this.canvas.beginPath();
        this.canvas.moveTo(37.5, 0);
        this.canvas.quadraticCurveTo(52.5, 0, 52.5, 15);
        this.canvas.lineTo(52.5, 25);
        this.canvas.quadraticCurveTo(52.5, 40, 37.5, 40);
        this.canvas.lineTo(16, 40);
        this.canvas.quadraticCurveTo(0, 40, 0, 25);
        this.canvas.lineTo(0, 15);
        this.canvas.quadraticCurveTo(0, 0, 15, 0);
        this.canvas.closePath();
        this.canvas.stroke();
        this.canvas.restore();
    }
    
    
    draw(gaze, pupil)
    {
        pupil[0] = Math.min(pupil[0], 1);
        pupil[0] = Math.max(pupil[0], 0);
        pupil[1] = Math.min(pupil[1], 1);
        pupil[1] = Math.max(pupil[1], 0);

        let w = this.width;
        let h = this.height;
        let s = Math.min(this.width, this.height)/180;
        let mw = Math.floor(0.5*(w-s*160))+0.5;
        let mh = Math.floor(0.5*(h-s*165))+0.5;

        this.canvas.setTransform(s, 0, 0, s, mw, mh);
        this.canvas.clearRect(0, 0, this.width, this.height);
        this.canvas.lineWidth = 1;

        // ears
        
        this.setColor(0);
        this.canvas.fillStyle = this.parameters.earColor;
 
        this.canvas.beginPath();
        this.canvas.rect(5, 60, 10, 65);
        this.canvas.rect(145, 60, 10, 65);
        this.canvas.fill();
        this.canvas.stroke();
        
        // head

        this.setColor(0);
        this.canvas.beginPath();

        this.canvas.moveTo(50, 0);
        this.canvas.lineTo(110, 0);
        this.canvas.quadraticCurveTo(145, 0, 145, 50);
        this.canvas.lineTo(145, 126);
        this.canvas.quadraticCurveTo(145, 165, 110, 165);
        this.canvas.lineTo(50, 165);
        this.canvas.quadraticCurveTo(15, 165, 15, 125);
        this.canvas.lineTo(15, 40);
        this.canvas.quadraticCurveTo(15, 0, 50, 0);
        this.canvas.closePath();
        
        this.canvas.moveTo(5, 62.5);
        this.canvas.lineTo(0, 67.5);
        this.canvas.lineTo(0, 117.5);
        this.canvas.lineTo(5, 122.5);
        this.canvas.closePath();

        this.canvas.moveTo(5, 62.5);
        this.canvas.lineTo(0, 67.5);
        this.canvas.lineTo(0, 117.5);
        this.canvas.lineTo(5, 122.5);
        this.canvas.closePath();
        
        this.canvas.moveTo(155, 62.5);
        this.canvas.lineTo(160, 67.5);
        this.canvas.lineTo(160, 117.5);
        this.canvas.lineTo(155, 122.5);
        this.canvas.closePath();
        
        this.canvas.fill();
        this.canvas.stroke();

        // Mouth
        
        this.canvas.fillStyle = this.parameters.mouthColor;
        for(let c=0; c<2; c++)
            for(let i=0; i<6; i++)
            {
                this.canvas.beginPath();
                this.canvas.arc(80-2.5*6.5+i*6.5, 135+c*10, 2, 0, 2*Math.PI);
                this.canvas.fill();
                this.canvas.stroke();
            }
        
         // left eye
        
        let g0 = gaze ? gaze[0] : parseFloat(this.parameters.gaze)+parseFloat(this.parameters.vergence);
        let p0 = pupil ? pupil[0] : this.parameters.pupil;
        this.drawEye(22.5, 72.5, g0, p0);

        // right eye

        let g1 = gaze ? gaze[1] : parseFloat(this.parameters.gaze)-parseFloat(this.parameters.vergence);
        let p1 = pupil ? pupil[1] : this.parameters.pupil;
        this.drawEye(85, 72.5, g1, p1);
    }


    update(d)
    {
        try {
            let gm = this.parameters['gazeModule'];
            let g = this.parameters['gazeSource'];
            let pm = this.parameters['pupilModule'];
            let p = this.parameters['pupilSource'];
            let gaze = null;
            let pupil = null;

            try {
                gaze = d[gm][g][0];
                if(len(gaze)<2)
                    gaze = [gaze[0], gaze[0]]
            }
            catch(err)
            {}
            
            try {
                pupil = d[pm][p][0];
                if(len(pupil)<2)
                    gaze = [pupil[0], pupil[0]]
            }
            catch(err)
            {}
            
            
            this.draw(gaze, pupil);
        }
        catch(err)
        {
            this.draw();
        }
    }
};


webui_widgets.add('webui-widget-epi-head', WebUIWidgetEpiHead);
