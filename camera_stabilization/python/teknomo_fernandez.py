import numpy as np
import cv2
from threading import Thread

from timable import ITimable
from utils import FixedSizeList


class TeknomoFernandez(ITimable, FixedSizeList):
    """
    Calculates the background of a set of images based on the Teknomo-Fernandez algorithm.

    https://en.wikipedia.org/wiki/Teknomo%E2%80%93Fernandez_algorithm
    """

    def __init__(self, levels: int = 6, history: int = 200, random_history: bool = True,
                 verbose: bool = False):
        """
        constructor

        :param levels: The number of levels during the algorithm
        :param history: The number of images in the history buffer to calculate the background from
        :param random_history: Flag if the elements in the history are removed randomly if the buffer is filled
        :param verbose: Print the timings of the steps of the algorithm
        """
        ITimable.__init__(self, name='Teknomo-Fernandez')
        FixedSizeList.__init__(self, max_num_elements=history, remove_random=random_history)
        self.levels = levels
        self.backgrounds_buffer = []
        self.background_calculation_buffer = []
        self.backgrounds = []
        self.verbose = verbose
        self.recalculation_allowed = True

    def append(self, value: any) -> None:
        super().append(value)
        self.calculate_async()

    def calculate_background_image_on_history(self, indices, use_gpu: bool = True):
        """
        Calculates the background image based on the Teknomo-Fernandez algorithm over the image history.

        :return: img3 * (img1 ^ img2) + img1 * img2
        """
        if use_gpu:
            model = cv2.cuda.bitwise_or(
                cv2.cuda.bitwise_and(self[indices[2]], (cv2.cuda.bitwise_xor(self[indices[0]], self[indices[1]]))),
                cv2.cuda.bitwise_and(self[indices[0]], self[indices[1]]))
        else:
            model = self[indices[2]] * (self[indices[0]] ^ self[indices[1]]) + self[indices[0]] * self[indices[1]]
        return model

    def calculate_background_image_on_background_buffer(self, indices, use_gpu: bool = True):
        """
        Calculates the background image based on the Teknomo-Fernandez algorithm over the image history.

        :return: img3 * (img1 ^ img2) + img1 * img2
        """
        if use_gpu:
            model = cv2.cuda.bitwise_or(cv2.cuda.bitwise_and(self.background_calculation_buffer[indices[2]], (
                cv2.cuda.bitwise_xor(self.background_calculation_buffer[indices[0]],
                                     self.background_calculation_buffer[indices[1]]))),
                                       cv2.cuda.bitwise_and(self.background_calculation_buffer[indices[0]],
                                                            self.background_calculation_buffer[indices[1]]))
        else:
            model = self.background_calculation_buffer[indices[2]] * (
                self.background_calculation_buffer[indices[0]] ^ self.background_calculation_buffer[indices[1]]) + \
               self.background_calculation_buffer[indices[0]] * self.background_calculation_buffer[indices[1]]
        return model

    def get_background(self):
        """
        Gets the latest calculated background image.

        :return: The latest background image if enough images are in the history,
                    The latest added frame if not,
                    None if no frames are added yet
        """
        if len(self.backgrounds) >= 1:
            return self.backgrounds[-1]
        if self.__len__() == 0:
            return None
        return self[-1]

    def reset_buffers(self):
        """
        Resets the background calculation buffers.
        """
        self.backgrounds_buffer = []
        self.background_calculation_buffer = []

    def calculation(self):
        """
        Performs calculation of the Teknomo-Fernandez algorithm on the current set of images.
        """
        if len(self) < 3:
            return

        self.recalculation_allowed = False
        self.add_timestamp('start')
        self.reset_buffers()

        indices = [np.random.randint(low=0, high=(len(self) - 1), size=3) for _ in range(3 ** (self.levels - 1))]
        self.background_calculation_buffer = [self.calculate_background_image_on_history(index) for index in indices]
        self.backgrounds_buffer.append(self.background_calculation_buffer[-1])
        self.add_timestamp('level 1')

        for i in range(2, self.levels + 1):
            indices = [[3 * j, 3 * j + 1, 3 * j + 2] for j in range(3 ** (self.levels - i))]
            results = [self.calculate_background_image_on_background_buffer(index) for index in indices]
            for x in range(len(results)):
                self.background_calculation_buffer[x] = results[x]
            self.backgrounds_buffer.append(self.background_calculation_buffer[0])
            self.add_timestamp('level {}'.format(i))

        if self.verbose:
            print(self.durations_to_str(reset=True))
        self.backgrounds = self.backgrounds_buffer
        self.recalculation_allowed = True

    def calculate_async(self):
        """
        Tries to start the calculation of the Teknomo-Fernandez algorithm on the current set of images.
        If no calculation is currently running it starts a new process, else does nothing.
        """
        if not self.recalculation_allowed:
            return
        Thread(target=self.calculation).start()

    def calculate_teknomo_fernandez_segmentation(self, kernel=np.ones((5, 5), np.uint8), diameter=9,
                                                 sigma_color=75, sigma_space=75, dilate_iterations=(3, 3),
                                                 bitmask_threshold=0.5):
        current_frame = self[-1]
        self.calculate_async()
        background = self.get_background()

        foreground_bitmask = cv2.dilate(
            cv2.bilateralFilter(
                cv2.morphologyEx(current_frame - background, cv2.MORPH_OPEN, kernel)
                , diameter, sigma_color, sigma_space)
            , kernel, iterations=dilate_iterations[0])

        foreground_bitmask = np.sum(foreground_bitmask / 3, axis=2, dtype=np.float32) / 255.
        foreground_bitmask = np.where(foreground_bitmask < bitmask_threshold, 0.0, 1.0)

        foreground_bitmask = cv2.dilate(foreground_bitmask, kernel, iterations=dilate_iterations[1])[..., None]
        foreground_bitmask = foreground_bitmask

        foreground = current_frame * foreground_bitmask / 255.0
        background_bitmask = (~np.array(foreground_bitmask, dtype=np.bool)).astype(np.float32)
        background = current_frame * background_bitmask / 255.0

        return foreground, foreground_bitmask, background, background_bitmask
