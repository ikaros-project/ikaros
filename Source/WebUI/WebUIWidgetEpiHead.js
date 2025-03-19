class WebUIWidgetEpiHead extends WebUIWidgetGraph {
  static template() {
    return [
      { name: "SOURCE", control: "header" },
      {
        name: "title",
        default: "Epi Head",
        type: "string",
        control: "textedit",
      },
      {
        name: "EyeDirection",
        default: "",
        type: "source",
        control: "textedit",
      },
      {
        name: "pupilSize",
        default: "",
        type: "source",
        control: "textedit",
      },
      {
        name: "LeftEyeColor",
        default: "",
        type: "source",
        control: "textedit",
      },
      {
        name: "RightEyeColor",
        default: "",
        type: "source",
        control: "textedit",
      },
      {
        name: "TopMouthColor",
        default: "",
        type: "source",
        control: "textedit",
      },
      {
        name: "LowMouthColor",
        default: "",
        type: "source",
        control: "textedit",
      },
      { name: "DEFAULT", control: "header" },

      {
        name: "EyeColor",
        default: "#88aaff",
        type: "string",
        control: "textedit",
      },
      {
        name: "MouthColor",
        default: "#000000",
        type: "string",
        control: "textedit",
      },
      {
        name: "Gaze",
        default: 0,
        type: "float",
        control: "slider",
        min: -45,
        max: 45,
      },
      {
        name: "Vergence",
        default: 0,
        type: "float",
        control: "slider",
        min: -20,
        max: 20,
      },
      {
        name: "pupilInMM",
        default: 11,
        type: "float",
        control: "slider",
        min: 6,
        max: 16,
      },
      {
        name: "headPosition",
        default: "",
        type: "source",
        control: "textedit",
      },
      {
        name: "EpiName",
        default: "EpiRed",
        type: "string",
        control: "textedit",
      },
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
    if (!/^#[0-9A-F]{6}$/i.test(epiColor)) {
      epiColor = "black"; // default color if invalid
    }

    // right ear
    this.canvas.fillStyle = epiColor;
    this.canvas.beginPath();
    this.canvas.rect(3, 60, 12, 65);
    this.canvas.fill();
    this.canvas.stroke();

    // left ear
    this.canvas.fillStyle = epiColor;
    this.canvas.beginPath();
    this.canvas.rect(145, 60, 12, 65);
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
    this.canvas.fill();
    this.canvas.stroke();

    this.drawMouth = function (isTopLeds) {
      const sourceIndex = isTopLeds ? 0 : 1;
      const yOffset = isTopLeds ? 135 : 145;

      for (let i = 0; i < 6; i++) {
        this.canvas.beginPath();
        this.canvas.arc(80 - 2.5 * 6.5 + i * 6.5, yOffset, 2, 0, 2 * Math.PI);
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
      const eyeOffsetX = isRightEye ? 85 : 22.5;
      const eyeOffsetY = 72.5;

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
      this.canvas.clip();

      // Gaze
      this.canvas.translate(27.5, 20); // Translate to center of the eye

      this.canvas.translate(
        27.5 * Math.sin((this.gaze[sourceIndex] * Math.PI) / 180),
        0
      ); // Change angle
      const scaleX = 1 - Math.abs(this.gaze[sourceIndex] * 0.006); // add angle effect on pupil. Assuming angle input max 45 degrees.
      this.canvas.transform(scaleX, 0, 0, 1, 0, 0);

      // Pupil
      this.canvas.fillStyle = "black";
      this.canvas.beginPath();
      this.canvas.arc(0, 0, this.pupil[sourceIndex] * 0.6, 0, 2 * Math.PI);
      this.canvas.fill();

      // Draw the NeoPixel ring
      const neoPixelRing = [];
      const centerX = 0;
      const centerY = 0;

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

      // Add Led diffuser
      this.canvas.fillStyle = "rgba(255, 255, 255, 0.8)";
      this.canvas.beginPath();
      this.canvas.arc(centerX, centerY, 18, 0, 2 * Math.PI);
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
    // let v = this.getSource("visibleSource");
    // if (v && v[0][0] == 0) {
    //     this.canvas.clearRect(-1, -1, this.width + 1, this.height + 1);
    //     return;
    // }


    // This needs to be in the same format as source. 2x8x3
    // And figure out the best color format. Maybe tranform rgb to #hex?

    const rgbToHex = (r, g, b) => {
      return "#" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
    };

    // Get values from source input or fill it with default values (can be an array).
    // Eyes
    let defaultEyeColor = this.parameters.EyeColor.split(',');
    if (defaultEyeColor.length == 12)
         this.EyeColors = [defaultEyeColor, defaultEyeColor];
    else
        this.EyeColors = [Array(12).fill(defaultEyeColor[0]), Array(12).fill(defaultEyeColor[0])];
    let l_eye = this.getSource("LeftEyeColor", "gray");
    let r_eye = this.getSource("RightEyeColor", "gray");
    // Convert the input to a hex representation of color 
    if (l_eye && l_eye.length === 3 && l_eye[0].length === 12 && l_eye[1].length === 12 && l_eye[2].length === 12 )
      this.EyeColors[0] = l_eye[0].map((_, i) => rgbToHex(Math.round(l_eye[0][i] * 255), Math.round(l_eye[1][i] * 255), Math.round(l_eye[2][i] * 255)));
    if (r_eye && r_eye.length === 3 && r_eye[0].length === 12 && r_eye[1].length === 12 && r_eye[2].length === 12 )
      this.EyeColors[1] = r_eye[0].map((_, i) => rgbToHex(Math.round(r_eye[0][i] * 255), Math.round(r_eye[1][i] * 255), Math.round(r_eye[2][i] * 255)));
    
    // Mouth
    let defaultMouthColor = this.parameters.MouthColor.split(',');
    if (defaultMouthColor.length == 8)
         this.MouthColors = [defaultMouthColor, defaultMouthColor];
    else
        this.MouthColors = [Array(8).fill(defaultMouthColor[0]), Array(8).fill(defaultMouthColor[0])];
    let t_mouth = this.getSource("TopMouthColor", "gray");
    let l_mouth = this.getSource("LowMouthColor", "gray");
    // Convert the input to a hex representation of color
    if (t_mouth && t_mouth.length === 3 && t_mouth[0].length === 12 && t_mouth[1].length === 12 && t_mouth[2].length === 12 )
      this.MouthColors[0] = t_mouth[0].map((_, i) => rgbToHex(Math.round(t_mouth[0][i] * 255), Math.round(t_mouth[1][i] * 255), Math.round(t_mouth[2][i] * 255)));
    if (l_mouth && l_mouth.length === 3 && l_mouth[0].length === 12 && l_mouth[1].length === 12 && l_mouth[2].length === 12 )
      this.MouthColors[1] = l_mouth[0].map((_, i) => rgbToHex(Math.round(l_mouth[0][i] * 255), Math.round(l_mouth[1][i] * 255), Math.round(l_mouth[2][i] * 255)));

    // Gaze. Only x wise and setting Gaze in webUI will controll both eyes.
    let defaultGaze = [
      parseFloat(this.parameters.Gaze) - parseFloat(this.parameters.Vergence),
      parseFloat(this.parameters.Gaze) + parseFloat(this.parameters.Vergence),
    ];
    this.gaze = this.getSource("Gaze", defaultGaze);
    if (this.gaze.length < 2) this.gaze = [this.gaze[0], this.gaze[0]];
    
    // Gaze. Only x wise and setting Gaze in webUI will controll both eyes. // What happens if source input is only one element?
    let defaultPupil = [parseFloat(this.parameters.pupilInMM),parseFloat(this.parameters.pupilInMM)];
    this.pupil = this.getSource("pupilSize", defaultPupil);
    if (this.pupil.length < 2) 
      this.pupil = [this.pupil[0], this.pupil[0]];

    // Head position. Fake tilt and pan of the robot. One value is treated as only tilt and two values tilt and pan.
    this.headPosition = this.getSource("headPosition", [0, 0]);
    if (this.headPosition.length < 2)
      this.headPosition = [this.headPosition[0], 0];

// 1. Add defualt values in template.
// 2. Check if input source is only one element.
// 3. Consistant naming
// 4. Check propotions of the head against fusion 360
// 5. Let Copliot find improvments of the code.
// 6. Fix select in slider. 

    this.draw();
  }
}

webui_widgets.add("webui-widget-epi-head", WebUIWidgetEpiHead);
