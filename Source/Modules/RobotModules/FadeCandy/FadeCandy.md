<?xml version="1.0"?>

<class name="FadeCandy" description="Allows control of fadecandy hardware on Epusing Jad Haj Mustafa fadecandy driver">

	<description type="text">

	</description>


 	<parameter name="simulate" type="bool" default="False" description="Simulation mode. Module is not trying to connect to any fadecandy board" />

	<input name="LEFT_EYE" description="RGB channels for the eye" />
	<input name="RIGHT_EYE" description="RGB channels for the eye" />
	<input name="MOUTH_HIGH" description="RGB channels for the mouth" />
	<input name="MOUTH_LOW" description="RGB channels for the mouth" />

	<link class="FadeCandy" />

	<author>
		<name>Birger Johansson</name>
		<email>birger.johansson@lucs.lu.se</email>
		<affiliation>Lund University Cognitive Science</affiliation>
	</author>

   <files>
   		<file>FadeCandy.h</file>
   		<file>FadeCandy.cc</file>
   		<file>FadeCandy.ikc</file>
   </files>
</class>