# -*- coding: iso-8859-1 -*-
# Maintainer: joaander

from hoomd import *
from hoomd import deprecated
from hoomd import md;
context.initialize()
import unittest
import os

# md.pair.gauss
class pair_gauss_tests (unittest.TestCase):
    def setUp(self):
        print
        deprecated.init.create_random(N=100, phi_p=0.05);
        self.nl = md.nlist.cell()
        context.current.sorter.set_params(grid=8)

    # basic test of creation
    def test(self):
        gauss = md.pair.gauss(r_cut=3.0, nlist = self.nl);
        gauss.pair_coeff.set('A', 'A', epsilon=1.0, sigma=1.0);
        gauss.update_coeffs();

    # test missing coefficients
    def test_set_missing_epsilon(self):
        gauss = md.pair.gauss(r_cut=3.0, nlist = self.nl);
        gauss.pair_coeff.set('A', 'A', sigma=1.0);
        self.assertRaises(RuntimeError, gauss.update_coeffs);

    # test missing coefficients
    def test_missing_AA(self):
        gauss = md.pair.gauss(r_cut=3.0, nlist = self.nl);
        self.assertRaises(RuntimeError, gauss.update_coeffs);

    # test set params
    def test_set_params(self):
        gauss = md.pair.gauss(r_cut=3.0, nlist = self.nl);
        gauss.set_params(mode="no_shift");
        gauss.set_params(mode="shift");
        gauss.set_params(mode="xplor");
        self.assertRaises(RuntimeError, gauss.set_params, mode="blah");

    # test specific nlist subscription
    def test_nlist_subscribe(self):
        gauss = md.pair.gauss(r_cut=2.5, nlist = self.nl);

        gauss.pair_coeff.set('A', 'A', sigma=1.0, epsilon=1.0)
        self.nl.update_rcut();
        self.assertAlmostEqual(2.5, self.nl.r_cut.get_pair('A','A'));

        gauss.pair_coeff.set('A', 'A', r_cut = 2.0)
        self.nl.update_rcut();
        self.assertAlmostEqual(2.0, self.nl.r_cut.get_pair('A','A'));

    def tearDown(self):
        del self.nl
        context.initialize();


if __name__ == '__main__':
    unittest.main(argv = ['test.py', '-v'])
