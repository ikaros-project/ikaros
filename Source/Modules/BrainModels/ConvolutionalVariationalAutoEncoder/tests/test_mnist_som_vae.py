"""Numerical and protocol tests for the offline SOM-VAE experiment."""

import unittest

import numpy as np

import run_mnist_som_vae as som


class SomVaeTests(unittest.TestCase):
    def setUp(self):
        rng = np.random.default_rng(19)
        self.x = rng.uniform(0.05, .95, (4, 5))
        self.model = {"encoder": rng.normal(0, .3, (5, 3)),
                      "encoder_bias": rng.normal(0, .3, 3),
                      "decoder": rng.normal(0, .3, (3, 5)),
                      "decoder_bias": rng.normal(0, .3, 5)}
        self.centers = rng.normal(0, .5, (4, 3))
        _, self.adjacency = som.grid(2)

    def test_full_gradient_without_neighbourhood(self):
        _, gradients, dc = som.loss_gradient(self.x, self.model, self.centers, self.adjacency, .1, 0)
        parameters = self.model | {"centers": self.centers}
        gradients["centers"] = dc
        for name, parameter in parameters.items():
            for index in np.ndindex(parameter.shape):
                old = parameter[index]
                parameter[index] = old + 1e-5
                plus = sum(som.loss_gradient(self.x, self.model, self.centers, self.adjacency, .1, 0)[0].values())
                parameter[index] = old - 1e-5
                minus = sum(som.loss_gradient(self.x, self.model, self.centers, self.adjacency, .1, 0)[0].values())
                parameter[index] = old
                self.assertAlmostEqual(gradients[name][index], (plus - minus) / 2e-5, places=7)

    def test_neighbour_gradient_and_stop_gradient(self):
        z = som.encode(self.x, self.model)
        _, dz, dc, _ = som.som_loss_gradient(z, self.centers, self.adjacency, 0, .1)
        np.testing.assert_array_equal(dz, 0)
        for index in np.ndindex(self.centers.shape):
            old = self.centers[index]
            self.centers[index] = old + 1e-5
            plus = sum(som.som_loss_gradient(z, self.centers, self.adjacency, 0, .1)[0].values())
            self.centers[index] = old - 1e-5
            minus = sum(som.som_loss_gradient(z, self.centers, self.adjacency, 0, .1)[0].values())
            self.centers[index] = old
            self.assertAlmostEqual(dc[index], (plus - minus) / 2e-5, places=7)

    def test_grid_does_not_wrap(self):
        _, adjacency = som.grid(3)
        np.testing.assert_array_equal(adjacency.sum(axis=1), [2, 3, 2, 3, 4, 3, 2, 3, 2])
        self.assertEqual(adjacency[0, 2], 0)
        self.assertEqual(adjacency[2, 3], 0)

    def test_empty_cell_label_uses_training_only(self):
        centers = np.array([[0., 0], [1., 0], [4., 0]])
        labels, counts = som.label_map(np.array([0, 0, 2]), np.array([3, 3, 7]), centers)
        np.testing.assert_array_equal(labels, [3, 3, 7])
        np.testing.assert_array_equal(counts.sum(axis=1), [2, 0, 1])

    def test_standardization_preserves_decoder(self):
        z = som.encode(self.x, self.model)
        mean, scale = z.mean(axis=0), z.std(axis=0)
        converted = {"encoder": self.model["encoder"] / scale,
                     "encoder_bias": (self.model["encoder_bias"] - mean) / scale,
                     "decoder": scale[:, None] * self.model["decoder"],
                     "decoder_bias": mean @ self.model["decoder"] + self.model["decoder_bias"]}
        np.testing.assert_allclose(som.decode(som.encode(self.x, converted), converted), som.decode(z, self.model))


if __name__ == "__main__":
    unittest.main()
