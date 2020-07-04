# Road radius detector

This tool implements the [RALPH system](https://www.ri.cmu.edu/pub_files/pub2/pomerleau_dean_1995_2/pomerleau_dean_1995_2.pdf) along with a TensorFlow model to detect the radius of a road on an image based on the trend taken by lines parallel to the street. It should thus be able to even recognize the curve of snowy roads, since those still have parallel features. The only parameters that need to be fed to the system are camera inclination, camera height from the ground, and camera FOV.

The repository also contains a flexible OpenGL generator for street images, used to verify whether the RALPH implementation works as expected and to train the model (labelled datasets complete with road radius could not be found).
