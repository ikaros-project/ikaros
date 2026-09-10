"""Behavioral checks against a built Ikaros executable; uses only synthetic signals."""

import ast
import math
from pathlib import Path
import re
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[5]
BINARY = ROOT / "Bin" / "ikaros"
GAINS = [1, 1, 0, 0, 0, -1, -1, -1, 1, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 1, 0, 0, 0, 0, 1, -1, 1]


def samples(text):
    result = []
    for match in re.finditer(r"INPUT =\s*(\{)", text):
        start = match.start(1)
        depth = 0
        for end in range(start, len(text)):
            depth += (text[end] == "{") - (text[end] == "}")
            if depth == 0:
                result.append(ast.literal_eval(text[start:end + 1].replace("{", "[").replace("}", "]")))
                break
    return result


def run_model(parameters=None, inputs=None, output="GLUTAMATE", ticks=1,
              dt=0.01, oscillator=False, chain=False):
    group = ET.Element("group", name="NeuralTissueTest", agent="Codex: GPT-6 Astra Medium",
                       tick_duration=str(dt))
    ET.SubElement(group, "module", name="Tissue", **{"class": "NeuralTissue"},
                  **(parameters or {}))
    for index, (port, data) in enumerate((inputs or {}).items()):
        name = f"Source{index}"
        ET.SubElement(group, "module", name=name, **{"class": "Constant"}, data=data)
        ET.SubElement(group, "connection", source=f"{name}.OUTPUT", target=f"Tissue.{port}", delay="0")
    if oscillator:
        ET.SubElement(group, "module", name="Pulse", **{"class": "Oscillator"}, type="square", frequency="1")
        ET.SubElement(group, "connection", source="Pulse.OUTPUT", target="Tissue.EXCITATION", delay="0")
    source = f"Tissue.{output}"
    if chain:
        ET.SubElement(group, "module", name="Second", **{"class": "NeuralTissue"}, glutamate_shape="AMPA.shape")
        ET.SubElement(group, "connection", source=source, target="Second.AMPA", delay="0")
        source = "Second.GLUTAMATE"
    ET.SubElement(group, "module", name="Probe", **{"class": "Print"})
    ET.SubElement(group, "connection", source=source, target="Probe.INPUT", delay="0")
    with tempfile.TemporaryDirectory(prefix="ikaros-neural-tissue-") as directory:
        path = Path(directory) / "test.ikg"
        ET.ElementTree(group).write(path, encoding="unicode")
        run = subprocess.run([str(BINARY), "-b", "-t", "1", "-s", str(ticks), str(path)],
                             cwd=ROOT, capture_output=True, text=True, timeout=30)
    return run.returncode, run.stdout + run.stderr


class NeuralTissueTests(unittest.TestCase):
    def values(self, **kwargs):
        code, text = run_model(**kwargs)
        self.assertEqual(code, 0, text)
        values = samples(text)
        self.assertTrue(values, text)
        return values

    def test_spatial_pattern_and_input_derived_shape(self):
        values = self.values(parameters={"glutamate_shape": "EXCITATION.shape"},
                             inputs={"EXCITATION": "[[0,0.25],[0.5,1]]"})[0]
        self.assertEqual([len(row) for row in values], [2, 2])
        alpha = 1 - math.exp(-0.1)
        for actual, source in zip(sum(values, []), [0, 0.25, 0.5, 1]):
            self.assertAlmostEqual(actual, alpha * source, places=6)

    def test_rank_three(self):
        values = self.values(parameters={"glutamate_shape": "AMPA.shape", "glutamate_tau": "0.000001"},
                             inputs={"AMPA": "[[[0,0.25],[0.5,1]],[[1,0.5],[0.25,0]]]"})[0]
        self.assertEqual(values, [[[0, 0.25], [0.5, 1]], [[1, 0.5], [0.25, 0]]])

    def test_fixed_shape_with_rectangular_internal_weights(self):
        values = self.values(parameters={"glutamate_shape": "3", "glutamate_tau": "0.000001",
                                        "glutamate_weights": "[[1,0],[0,1],[0.5,0.5]]"},
                             inputs={"EXCITATION": "[0.2,0.8]"})[0]
        self.assertEqual(values, [0.2, 0.8, 0.5])

    def test_shunting_and_inhibition(self):
        value = self.values(inputs={"EXCITATION": "[1]", "INHIBITION": "[0.25]",
                                    "SHUNTING_INHIBITION": "[1]"})[0][0]
        self.assertAlmostEqual(value, 0.25 * (1 - math.exp(-0.1)), places=6)

    def test_independent_transmitter_population(self):
        parameters = {"gaba_enabled": "yes", "gaba_tau": "0.2", "gaba_shape": "2",
                      "gaba_weights": "[[1],[0.5]]"}
        values = self.values(parameters=parameters, inputs={"EXCITATION": "[1]"}, output="GABA")[0]
        self.assertAlmostEqual(values[0], 1 - math.exp(-0.05), places=6)
        self.assertAlmostEqual(values[1], 0.5 * (1 - math.exp(-0.05)), places=6)
        glutamate = self.values(parameters=parameters, inputs={"EXCITATION": "[1]"})[0]
        self.assertAlmostEqual(glutamate[0], 1 - math.exp(-0.1), places=6)

    def test_configurable_receptor_gain(self):
        gains = GAINS.copy()
        gains[11] = 0.5
        values = self.values(parameters={"glutamate_gains": str(gains), "glutamate_tau": "0.000001"},
                             inputs={"D1_LIKE": "[1]"})
        self.assertEqual(values, [[0.5]])

    def test_missing_inputs_and_disabled_output(self):
        self.assertEqual(self.values(ticks=2), [[0], [0]])
        self.assertEqual(self.values(inputs={"EXCITATION": "[1]"}, output="GABA"), [[0]])

    def test_large_timestep_remains_bounded(self):
        self.assertEqual(self.values(inputs={"EXCITATION": "[100]"}, dt=10), [[1]])

    def test_two_module_signal_flow(self):
        value = self.values(inputs={"EXCITATION": "[1]"}, chain=True)[0][0]
        self.assertAlmostEqual(value, (1 - math.exp(-0.1)) ** 2, places=6)

    def test_decay_after_pulse(self):
        values = [row[0] for row in self.values(oscillator=True, ticks=10, dt=0.1)]
        self.assertGreater(max(values), 0.9)
        self.assertGreater(values[7], 0)
        self.assertAlmostEqual(values[8] / values[7], math.exp(-1), places=5)

    def test_explicit_mapping_required_even_with_equal_element_count(self):
        code, text = run_model(parameters={"glutamate_shape": "4"},
                               inputs={"EXCITATION": "[[0,0],[0,0]]"})
        self.assertNotEqual(code, 0)
        self.assertIn("supply explicit internal weights", text)

    def test_invalid_configuration(self):
        cases = [({"glutamate_tau": "0"}, "require tau > 0"),
                 ({"glutamate_gains": "[1]"}, "exactly 26"),
                 ({"glutamate_weights": "[[1,1]]"}, "sum of input sizes"),
                 ({"glutamate_weights": "[[-1]]"}, "finite and nonnegative")]
        for parameters, message in cases:
            with self.subTest(parameters=parameters):
                code, text = run_model(parameters=parameters, inputs={"EXCITATION": "[1]"})
                self.assertNotEqual(code, 0)
                self.assertIn(message, text)

    def test_invalid_input(self):
        code, text = run_model(inputs={"EXCITATION": "[-1]"})
        self.assertIn("inputs must be finite and nonnegative", text)
        self.assertNotEqual(code, 0, text)


if __name__ == "__main__":
    unittest.main()
