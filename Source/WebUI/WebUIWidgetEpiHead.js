class WebUIWidgetEpiHead extends WebUIWidgetGraph {
  static template() {
    return [
      { name: "SOURCE", control: "header" },
      { name: "title", default: "Epi Head", type: "string", control: "textedit" },
      { name: "eyeDirection", default: "", type: "source", control: "textedit" },
      { name: "pupilSize", default: "", type: "source", control: "textedit" },
      { name: "leftEyeColor", default: "", type: "source", control: "textedit" },
      { name: "rightEyeColor", default: "", type: "source", control: "textedit" },
      { name: "topMouthColor", default: "", type: "source", control: "textedit" },
      { name: "lowMouthColor", default: "", type: "source", control: "textedit" },
      { name: "headPosition", default: "", type: "source", control: "textedit" },
      { name: "PARAMETERS", control: "header" },
      { name: "EyeColor", default: "#ffdd88", type: "string", control: "textedit" },
      { name: "MouthColor", default: "#ffdd88", type: "string", control: "textedit" },
      { name: "Gaze", default: 0, type: "float", control: "slider", min: -45, max: 45 },
      { name: "Vergence", default: 0, type: "float", control: "slider", min: -20, max: 20 },
      { name: "PupilInMM", default: 11, type: "float", control: "slider", min: 6, max: 16 },
      { name: "EpiName", default: "EpiRed", type: "string", control: "textedit" },
      { name: "STYLE", control: "header" },
      { name: "color", default: "black", type: "string", control: "textedit" },
      { name: "fill", default: "white", type: "string", control: "textedit" },
      { name: "show_title", default: false, type: "bool", control: "checkbox" },
      { name: "show_frame", default: false, type: "bool", control: "checkbox" },
      { name: "style", default: "", type: "string", control: "textedit" },
      { name: "frame-style", default: "", type: "string", control: "textedit" },
    ];
  }

  init() {
    super.init();

    // this.onclick = function () {
    //   alert(this.data);
    // }; // last matrix
  }

  draw() {
    let w = this.width;
    let h = this.height;
    let s = Math.min(this.width, this.height) / 180;
    let mw = Math.floor(0.5 * (w - s * 160)) + 0.5;
    let mh = Math.floor(0.5 * (h - s * 165)) + 0.5;

    this.canvas.clearRect(-1, -1, this.width + 1, this.height + 1);
    this.canvas.setTransform(s, 0, 0, s, mw, mh);
    this.canvas.lineWidth = 1;

    // Move head
    this.canvas.translate(
      27.5 * Math.sin((this.headPosition[0] * Math.PI) / 180),
      27.5 * Math.sin((this.headPosition[1] * Math.PI) / 180)
    );

    // Figure out color of Epi.
    let epiColor = this.parameters.EpiName.substring(3);

    // left ear
    this.canvas.fillStyle = epiColor;
    this.canvas.beginPath();
    this.canvas.moveTo(3, 60);  
    this.canvas.lineTo(13, 60);   
    this.canvas.lineTo(13, 60+68);   
    this.canvas.lineTo(3, 60+68);  
    this.canvas.lineTo(0, 60+68-5);  
    this.canvas.lineTo(0, 60+5);  
    this.canvas.closePath();
    this.canvas.fill();
    this.canvas.stroke();
    this.canvas.beginPath();
    this.canvas.moveTo(3, 60);  
    this.canvas.lineTo(3, 60+68);  
    this.canvas.closePath(); 
    this.canvas.fill();
    this.canvas.stroke();

    // right ear
    this.canvas.fillStyle = epiColor;
    this.canvas.beginPath();
    this.canvas.moveTo(13+131, 60);  
    this.canvas.lineTo(13+131+10, 60);   
    this.canvas.lineTo(13+131+10+2.5, 60+5);   
    this.canvas.lineTo(13+131+10+2.5, 60+68-5);   
    this.canvas.lineTo(13+131+10, 60+68);   
    this.canvas.lineTo(13+131, 60+68);   
    this.canvas.closePath();
    this.canvas.fill();
    this.canvas.stroke();
    this.canvas.beginPath();
    this.canvas.moveTo(13+131+10, 60);  
    this.canvas.lineTo(13+131+10, 60+68);  
    this.canvas.closePath(); 
    this.canvas.stroke();

    // head
    this.setColor(0);
    this.canvas.beginPath();
    this.canvas.moveTo(13+40, 0);                       // 53,0
    this.canvas.lineTo(13+40+51, 0);                    // 104,0
    this.canvas.quadraticCurveTo(144, 0, 144, 40);      // 144,40
    this.canvas.lineTo(144, 40+81);                     // 144,91
    this.canvas.quadraticCurveTo(144, 163, 144-40, 163);   // 104,163
    this.canvas.lineTo(13+40, 163);                     // 53,163
    this.canvas.quadraticCurveTo(13, 163, 13, 40+81);     // 13,144
    this.canvas.lineTo(13, 40);                         // 13,40
    this.canvas.quadraticCurveTo(13, 0, 13+40, 0);      // 53,0
    this.canvas.closePath();
    this.canvas.fill();
    this.canvas.stroke();

    this.drawMouth = function (isTopLeds) {
      const sourceIndex = isTopLeds ? 0 : 1;
      const yOffset = isTopLeds ? 133 : 145;

    // first and last led is not visible in Epi
    for (let i = 0; i < 8; i++) {
      if (i === 0 || i === 7) continue;
      this.canvas.beginPath();
      this.canvas.arc(77 - 3.25 * 6.3 + i * 6.3, yOffset, 2, 0, 2 * Math.PI); // 3.25?
      this.canvas.fillStyle = this.MouthColors[sourceIndex][i];
      this.canvas.fill();
      this.canvas.stroke();
    }
    };

    // Draw top mouth
    this.drawMouth(true);

    // Draw low mouth
    this.drawMouth(false);

    // Eyes
    // ====

    this.drawEye = function (isRightEye) {
      const sourceIndex = isRightEye ? 1 : 0;
      const eyeOffsetX = isRightEye ? 154-52.5+3-20 : 0+20;
      const eyeOffsetY = 71.5;

      // Eye outline
      this.canvas.save();
      this.canvas.translate(eyeOffsetX, eyeOffsetY);
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
      this.canvas.clip(); // Woll make it hard to click

      // Gaze
      this.canvas.translate(27.5, 20); // Translate to center of the eye

      this.canvas.translate(27.5 * Math.sin((this.gaze[sourceIndex] * Math.PI) / 180),0 ); // Change angle
      const scaleX = 1 - Math.abs(this.gaze[sourceIndex] * 0.006); // add angle effect on pupil. Assuming angle input max 45 degrees.
      this.canvas.transform(scaleX, 0, 0, 1, 0, 0);

      // Draw the NeoPixel ring
      const neoPixelRing = [];
      const centerX = 0;
      const centerY = 0;

    // Add Led diffuser
    this.canvas.fillStyle = "rgb(255, 255, 255)";
    this.canvas.beginPath();
    this.canvas.arc(centerX, centerY, 18, 0, 2 * Math.PI);
    this.canvas.stroke();
    this.canvas.fill();
      
      const radius = 8 + 5; // Radius of the iris plus some offset for the NeoPixel ring
      const ledRadius = 3.5;
      const numLEDs = 12;

      for (let i = 0; i < numLEDs; i++) {
        const angle = (i / numLEDs) * 2 * Math.PI;
        const x = centerX + radius * Math.cos(angle);
        const y = centerY + radius * Math.sin(angle);
        neoPixelRing.push({ x: x, y: y });
      }

      neoPixelRing.forEach((led, index) => {
        this.canvas.beginPath();
        this.canvas.arc(led.x, led.y, ledRadius, 0, 2 * Math.PI);
        this.canvas.fillStyle = this.EyeColors[sourceIndex][index];
        this.canvas.fill();
      });


          // Pupil
        this.canvas.fillStyle = "black";
        this.canvas.beginPath();
        this.canvas.arc(0, 0, this.pupil[sourceIndex] * 0.6, 0, 2 * Math.PI);
        this.canvas.fill();

      this.canvas.restore();



      // Draw eye outline
      this.canvas.save();
      this.canvas.translate(eyeOffsetX, eyeOffsetY);
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
    };

    // Draw left eye
    this.drawEye(false);

    // Draw right eye
    this.drawEye(true);
  }

  update() {

    const rgbToHex = (r, g, b) => {
      return "#" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
    };

    // Get values from source input or fill it with default values
    // Eyes
    let defaultEyeColor = this.parameters.EyeColor.split(','); // Is this suppose to get the default value from template?
    if (defaultEyeColor.length === 0 || (defaultEyeColor.length === 1 && defaultEyeColor[0] === '')) {
        defaultEyeColor = ['yellow'];
    }
    if (defaultEyeColor.length == 12)
         this.EyeColors = [defaultEyeColor, defaultEyeColor];
    else
        this.EyeColors = [Array(12).fill(defaultEyeColor[0]), Array(12).fill(defaultEyeColor[0])];
    
    let l_eye = this.getSource("LeftEyeColor", defaultEyeColor);
    let r_eye = this.getSource("RightEyeColor", defaultEyeColor);
    
    // Convert the input to a hex representation of color 
    if (l_eye && l_eye.length === 3 && l_eye[0].length === 12 && l_eye[1].length === 12 && l_eye[2].length === 12 )
      this.EyeColors[0] = l_eye[0].map((_, i) => rgbToHex(Math.round(l_eye[0][i] * 255), Math.round(l_eye[1][i] * 255), Math.round(l_eye[2][i] * 255)));
    if (r_eye && r_eye.length === 3 && r_eye[0].length === 12 && r_eye[1].length === 12 && r_eye[2].length === 12 )
      this.EyeColors[1] = r_eye[0].map((_, i) => rgbToHex(Math.round(r_eye[0][i] * 255), Math.round(r_eye[1][i] * 255), Math.round(r_eye[2][i] * 255)));
    
    // Mouth
    let defaultMouthColor = this.parameters.MouthColor.split(',');
    if (defaultMouthColor.length === 0 || (defaultMouthColor.length === 1 && defaultMouthColor[0] === '')) {
        defaultMouthColor = ['yellow'];
    }
    if (defaultMouthColor.length == 8)
         this.MouthColors = [defaultMouthColor, defaultMouthColor];
    else
        this.MouthColors = [Array(8).fill(defaultMouthColor[0]), Array(8).fill(defaultMouthColor[0])];
    let t_mouth = this.getSource("topMouthColor", defaultMouthColor);
    let l_mouth = this.getSource("lowMouthColor", defaultMouthColor);
    // Convert the input to a hex representation of color
    if (t_mouth && t_mouth.length === 3 && t_mouth[0].length === 8 && t_mouth[1].length === 8 && t_mouth[2].length === 8 )
      this.MouthColors[0] = t_mouth[0].map((_, i) => rgbToHex(Math.round(t_mouth[0][i] * 255), Math.round(t_mouth[1][i] * 255), Math.round(t_mouth[2][i] * 255)));
    if (l_mouth && l_mouth.length === 3 && l_mouth[0].length === 8 && l_mouth[1].length === 8 && l_mouth[2].length === 8 )
      this.MouthColors[1] = l_mouth[0].map((_, i) => rgbToHex(Math.round(l_mouth[0][i] * 255), Math.round(l_mouth[1][i] * 255), Math.round(l_mouth[2][i] * 255)));

    // Gaze. Only x wise and setting Gaze in webUI will controll both eyes.
    let defaultGaze = [
      parseFloat(this.parameters.Gaze) - parseFloat(this.parameters.Vergence),
      parseFloat(this.parameters.Gaze) + parseFloat(this.parameters.Vergence),
    ];
    this.gaze = this.getSource("eyeDirection", defaultGaze);
    if (this.gaze.length < 2) this.gaze = [this.gaze[0], this.gaze[0]];
    
    // Gaze. Only x wise and setting Gaze in webUI will controll both eyes. // What happens if source input is only one element?
    let defaultPupil = [parseFloat(this.parameters.PupilInMM),parseFloat(this.parameters.PupilInMM)];
    this.pupil = this.getSource("pupilSize", defaultPupil);
    if (this.pupil.length < 2) 
      this.pupil = [this.pupil[0], this.pupil[0]];

    // Head position. Fake tilt and pan of the robot. One value is treated as only tilt and two values tilt and pan.
    this.headPosition = this.getSource("HeadPosition", [0, 0]);

    if (this.headPosition.length < 2)
      this.headPosition = [this.headPosition[0], 0];
    this.draw();
  }
}

webui_widgets.add("webui-widget-epi-head", WebUIWidgetEpiHead);
