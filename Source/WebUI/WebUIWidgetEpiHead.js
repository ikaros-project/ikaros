class WebUIWidgetEpiHead extends WebUIWidgetGraph {
    static template() {
        return [
            { 'name': "EPI HEAD", 'control': 'header' },
            { 'name': 'title', 'default': "Epi Head", 'type': 'string', 'control': 'textedit' },
            { 'name': 'gazeSource', 'default': "", 'type': 'source', 'control': 'textedit' },
            { 'name': 'pupilLeftSource', 'default': "", 'type': 'source', 'control': 'textedit' },
            { 'name': 'pupilRightSource', 'default': "", 'type': 'source', 'control': 'textedit' },
            { 'name': 'headPosition', 'default': "", 'type': 'source', 'control': 'textedit' },

            { 'name': 'irisLeftRGB', 'default': "", 'type': 'source', 'control': 'textedit' },
            { 'name': 'irisRightRGB', 'default': "", 'type': 'source', 'control': 'textedit' },
            { 'name': 'mouthRGB', 'default': "", 'type': 'source', 'control': 'textedit' },

            { 'name': 'irisLeftIntensity', 'default': "", 'type': 'source', 'control': 'textedit' },
            { 'name': 'irisRightIntensity', 'default': "", 'type': 'source', 'control': 'textedit' },
            { 'name': 'mouthIntensity', 'default': "", 'type': 'source', 'control': 'textedit' },

            { 'name': 'motionRecorderInput', 'default': "", 'type': 'source', 'control': 'textedit' },

            { 'name': 'gaze', 'default': 0, 'type': 'float', 'control': 'slider', 'min': -45, 'max': 45 },
            { 'name': 'vergence', 'default': 0, 'type': 'float', 'control': 'slider', 'min': -20, 'max': 20 },
            { 'name': 'pupilInMM', 'default': 11, 'type': 'float', 'control': 'slider', 'min': 6, 'max': 16 },

            { 'name': 'visibleSource', 'default': "", 'type': 'source', 'control': 'textedit' },
            { 'name': 'visibleFace', 'default': true, 'type': 'bool', 'control': 'checkbox' },
            { 'name': 'visibleFaceParameter', 'default': "", 'type': 'source', 'control': 'textedit' },

            { 'name': "STYLE", 'control': 'header' },

            { 'name': 'color', 'default': "black", 'type': 'string', 'control': 'textedit' },
            { 'name': 'fill', 'default': "white", 'type': 'string', 'control': 'textedit' },
            { 'name': 'earColor', 'default': "#0088ff", 'type': 'string', 'control': 'textedit' },
            { 'name': 'irisColor', 'default': "#88aaff", 'type': 'string', 'control': 'textedit' },
            { 'name': 'mouthColor', 'default': "#000000", 'type': 'string', 'control': 'textedit' },
            { 'name': 'show_title', 'default': false, 'type': 'bool', 'control': 'checkbox' },
            { 'name': 'show_frame', 'default': false, 'type': 'bool', 'control': 'checkbox' },
            { 'name': 'style', 'default': "", 'type': 'string', 'control': 'textedit' },
            { 'name': 'frame-style', 'default': "", 'type': 'string', 'control': 'textedit' }
        ]
    };


    init() {
        super.init();

        this.onclick = function () { alert(this.data) }; // last matrix
    }


    drawEye(x, y, gaze, pupil, rgb, alpha) {
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

        // Should be matched with Epi!
        let v = 27.5 * Math.sin(gaze * Math.PI / 180)

        // Iris
        if (rgb)
            this.canvas.fillStyle = 'rgb('
                + Math.floor(255 * rgb[0]) + ','
                + Math.floor(255 * rgb[1]) + ','
                + Math.floor(255 * rgb[2]) + ','
                + alpha + ')'

        else
            this.canvas.fillStyle = this.parameters.irisColor; // when LED input not connected

        this.canvas.strokeStyle = "#00000000";
        this.canvas.beginPath();
        this.canvas.arc(27.5 + v, 20, 17.5, 0, 2 * Math.PI);
        this.canvas.fill();

        // Pupil

        this.canvas.fillStyle = "black";
        this.canvas.beginPath();
        this.canvas.arc(27.5 + v, 20, pupil / 18 * (17.5 * 0.5625), 0, 2 * Math.PI);
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


    draw(gaze, pupil, offset, iLRGB, iLI, iRRGB, iRI, mRGB, mI) {
        let w = this.width;
        let h = this.height;
        let s = Math.min(this.width, this.height) / 180;
        let mw = Math.floor(0.5 * (w - s * 160)) + 0.5;
        let mh = Math.floor(0.5 * (h - s * 165)) + 0.5;

        this.canvas.clearRect(-1, -1, this.width + 1, this.height + 1);
        this.canvas.setTransform(s, 0, 0, s, mw, mh);
        this.canvas.lineWidth = 1;

        this.canvas.translate(offset[0], offset[1]);

        let drawFace = this.parameters.visibleFace;
        if (this.parameters.visibleFaceParameter)
            drawFace = !!parseFloat(this.getSource("visibleFaceParameter"));

        if (drawFace) {
            // right ear
            this.canvas.fillStyle = "red";
            this.canvas.beginPath();
            this.canvas.rect(3, 60, 12, 65);
            this.canvas.fill();
            this.canvas.stroke();

            // left ear
            this.canvas.fillStyle = "blue";
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

            // Mouth
            if (mRGB)
                this.canvas.fillStyle = 'rgba('
                    + Math.floor(255 * mRGB[0]) + ','
                    + Math.floor(255 * mRGB[1]) + ','
                    + Math.floor(255 * mRGB[2]) + ','
                    + mI + ')'
            else
                this.canvas.fillStyle = this.parameters.mouthColor;

            // Stroke
            for (let c = 0; c < 2; c++)
                for (let i = 0; i < 6; i++) {
                    this.canvas.beginPath();
                    this.canvas.arc(80 - 2.5 * 6.5 + i * 6.5, 135 + c * 10, 2, 0, 2 * Math.PI);
                    this.canvas.fill();
                    this.canvas.stroke();
                }
        }

        // Eyes
        // ====

        // Left eyes
        // Eye outline
        this.canvas.save();
        this.canvas.translate(22.5, 72.5);
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

        // Should be matched with Epi!
        let v = 27.5 * Math.sin(gaze[1] * Math.PI / 180)

        // Pupil
        this.canvas.fillStyle = "black";
        this.canvas.beginPath();
        this.canvas.arc(27.5 + v, 20, pupil[1] / 18 * (17.5 * 0.5625), 0, 2 * Math.PI);
        this.canvas.fill();
        this.canvas.restore();

        // Draw the left NeoPixel ring
{
        const leftNeoPixelRing = [];
        const leftCenterX = 22.5 + 27.5;
        const leftCenterY = 72.5 + 20;
        const leftRadius = 8 + 5; // Radius of the iris plus some offset for the NeoPixel ring
        const leftLedRadius = 3.5;
        const leftNumLEDs = 12;

        for (let i = 0; i < leftNumLEDs; i++) {
            const leftAngle = (i / leftNumLEDs) * 2 * Math.PI;
            const leftX = leftCenterX + leftRadius * Math.cos(leftAngle);
            const leftY = leftCenterY + leftRadius * Math.sin(leftAngle);
            leftNeoPixelRing.push({ x: leftX, y: leftY });
        }

        leftNeoPixelRing.forEach(leftLed => {
            this.canvas.beginPath();
            this.canvas.arc(leftLed.x, leftLed.y, leftLedRadius, 0, 2 * Math.PI);
            this.canvas.fillStyle = '#00AFFF';
            this.canvas.fill();
        });

        // Add Led diffuser
        this.canvas.fillStyle = "rgba(255, 255, 255, 0.8)";
        this.canvas.beginPath();
        this.canvas.arc(leftCenterX, leftCenterY, 18, 0, 2 * Math.PI);
        this.canvas.fill();

        this.canvas.restore();

        // Draw outline again
        this.canvas.save();
        this.canvas.translate(22.5, 72.5);
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



        // Right eyes
        // Eye outline
        this.canvas.save();
        this.canvas.translate(85, 72.5);
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

        // Should be matched with Epi!
        v = 27.5 * Math.sin(gaze[1] * Math.PI / 180)

        // Iris
        // if (iLRGB)
        //     this.canvas.fillStyle = 'rgb('
        //         + Math.floor(255 * iLRGB[0]) + ','
        //         + Math.floor(255 * iLRGB[1]) + ','
        //         + Math.floor(255 * iLRGB[2]) + ','
        //         + iRI + ')'

        // else
        //     this.canvas.fillStyle = this.parameters.irisColor; // when LED input not connected

        // this.canvas.strokeStyle = "#00000000";
        // this.canvas.beginPath();
        // this.canvas.arc(27.5 + v, 20, 17.5, 0, 2 * Math.PI);
        // this.canvas.fill();

        // Pupil

        this.canvas.fillStyle = "black";
        this.canvas.beginPath();
        this.canvas.arc(27.5 + v, 20, pupil[1] / 18 * (17.5 * 0.5625), 0, 2 * Math.PI);
        this.canvas.fill();
        this.canvas.restore();

        // Draw the left NeoPixel ring

        const rightNeoPixelRing = [];
        const rightCenterX = 85 + 27.5;
        const rightCenterY = 72.5 + 20;
        const rightRadius = 8 + 5; // Radius of the iris plus some offset for the NeoPixel ring
        const rightLedRadius = 3.5;
        const rightNumLEDs = 12;

        for (let i = 0; i < rightNumLEDs; i++) {
            const rightAngle = (i / rightNumLEDs) * 2 * Math.PI;
            const rightX = rightCenterX + rightRadius * Math.cos(rightAngle);
            const rightY = rightCenterY + rightRadius * Math.sin(rightAngle);
            rightNeoPixelRing.push({ x: rightX, y: rightY });
        }

        rightNeoPixelRing.forEach(rightLed => {
            this.canvas.beginPath();
            this.canvas.arc(rightLed.x, rightLed.y, rightLedRadius, 0, 2 * Math.PI);
            this.canvas.fillStyle = '#00AFFF';
            this.canvas.fill();
        });

        // Add Led diffuser
        this.canvas.fillStyle = "rgba(255, 255, 255, 0.8)";
        this.canvas.beginPath();
        this.canvas.arc(rightCenterX, rightCenterY, 18, 0, 2 * Math.PI);
        this.canvas.fill();


        this.canvas.restore();


        // Draw outline again
        this.canvas.save();
        this.canvas.translate(85, 72.5);
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


    update() {
        let v = this.getSource('visibleSource');
        if (v && v[0][0] == 0) {
            this.canvas.clearRect(-1, -1, this.width + 1, this.height + 1);
            return;
        }

        // Special motion recorder input. Should be removed when we have better select in webUI source
        if (this.getSource('motionRecorderInput')) {
            let motionRecorderSource = this.getSourceAsArray('motionRecorderInput');
            let headPosition = [[0, 0]];
            let gaze = [motionRecorderSource[2], motionRecorderSource[3]];
            let pupil = [motionRecorderSource[4], motionRecorderSource[5]];
            let iLRGB = [motionRecorderSource[6], motionRecorderSource[7], motionRecorderSource[8]];
            let iRRGB = [motionRecorderSource[9], motionRecorderSource[10], motionRecorderSource[11]];
            let mRGB = [motionRecorderSource[12], motionRecorderSource[13], motionRecorderSource[14]];

            let iLI = [motionRecorderSource[15]];
            let iRI = [motionRecorderSource[16]];
            let mI = [motionRecorderSource[17]];

            if (gaze.length < 2)
                gaze = [gaze[0], gaze[0]]

            if (headPosition.length < 2)
                headPosition = [headPosition[0], 0]

            this.draw(gaze, pupil, headPosition, iLRGB, iLI, iRRGB, iRI, mRGB, mI);
        }
        else {
            let defaultGaze = [parseFloat(this.parameters.gaze) - parseFloat(this.parameters.vergence),
            parseFloat(this.parameters.gaze) + parseFloat(this.parameters.vergence)];
            let defaultPupil = parseFloat(this.parameters.pupilInMM);

            let gaze = this.getSourceAsArray('gazeSource', defaultGaze);
            let pupil = [this.getSourceAsFloat('pupilLeftSource', defaultPupil), this.getSourceAsFloat('pupilRightSource', defaultPupil)];
            let headPosition = this.getSource('headPosition', [[0, 0]]);
            let iLRGB = this.getSourceAsArray('irisLeftRGB');
            let iRRGB = this.getSourceAsArray('irisRightRGB');
            let mRGB = this.getSourceAsArray('mouthRGB');

            let iLI = this.getSourceAsFloat('irisLeftIntensity', [[1]]);
            let iRI = this.getSourceAsFloat('irisRightIntensity', [[1]]);
            let mI = this.getSourceAsFloat('mouthIntensity', [[1]]);

            if (gaze.length < 2)
                gaze = [gaze[0], gaze[0]]

            if (headPosition.length < 2)
                headPosition = [headPosition[0], 0]

            this.draw(gaze, pupil, headPosition, iLRGB, iLI, iRRGB, iRI, mRGB, mI);
            //this.draw();
        }

    }
};

webui_widgets.add('webui-widget-epi-head', WebUIWidgetEpiHead);

