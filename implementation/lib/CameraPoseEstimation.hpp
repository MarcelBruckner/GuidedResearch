//
// Created by brucknem on 02.02.21.
//

#ifndef CAMERASTABILIZATION_CAMERAPOSEESTIMATION_HPP
#define CAMERASTABILIZATION_CAMERAPOSEESTIMATION_HPP

#include <utility>
#include <vector>
#include <iostream>
#include <thread>
#include <limits>

#include "ceres/ceres.h"
#include "glog/logging.h"

#include "opencv2/opencv.hpp"
#include "Residuals.hpp"
#include "WorldObjects.hpp"

namespace providentia {
	namespace calibration {

		/**
		 * Prints a vector as a row.
		 */
		std::basic_string<char, std::char_traits<char>, std::allocator<char>> printVectorRow(Eigen::Vector3d vector);

		/**
		 * Estimates the camera pose from some known correspondences between the world and image.
		 */
		class CameraPoseEstimator {
		private:
			cv::RNG rng;
			std::vector<ceres::ResidualBlockId> correspondenceResiduals;
			std::vector<ceres::ResidualBlockId> weightResiduals;
			std::vector<ceres::ResidualBlockId> lambdaResiduals;
			std::vector<ceres::ResidualBlockId> rotationResiduals;

			/**
			 * The final optimization summary.
			 */
			ceres::Solver::Summary summary;

			/**
			 * The initial camera [x, y, z] translation in world space.
			 */
			Eigen::Vector3d initialTranslation;

			/**
			 * The current camera [x, y, z] translation in world space used for optimization.
			 */
			Eigen::Vector3d translation;

			/**
			 * The initial camera [x, y, z] euler angle rotation around the world space axis.
			 */
			Eigen::Vector3d initialRotation;

			/**
			 * The current camera [x, y, z] euler angle rotation around the world space axis.
			 */
			Eigen::Vector3d rotation;

			/**
			 * The intrinsics matrix of the pinhole camera model.
			 */
			Eigen::Matrix<double, 3, 4> intrinsics;

			/**
			 * A buffer for the known world worldObjects.
			 */
			std::vector<providentia::calibration::WorldObject> worldObjects;

			bool hasRotationGuess = false;
			bool hasTranslationGuess = false;

			bool optimizationFinished = true;

			double weightPenalizeScale = std::numeric_limits<double>::max();
			double lambdaPenalizeScale = 2;
			double rotationPenalizeScale = 50;

			double initialDistanceFromMean = 500;

			std::vector<double *> weights;
			double weightsScale = 1;

			bool foundValidSolution = false;
			int maxTriesUntilAbort = 15;

			double lambdasLoss = 0;
			double correspondencesLoss = 0;
			double rotationsLoss = 0;
			double weightsLoss = 0;
			double totalLoss = 0;

			/**
			 * Calculates the mean of the known world correspondences.
			 */
			Eigen::Vector3d calculateMean();

			ceres::Problem createProblem();

			Eigen::Vector3d calculateFurthestPoint(const Eigen::Vector3d &mean);

			static ceres::ScaledLoss *getScaledHuberLoss(double scale);

			static ceres::ScaledLoss *getScaledHuberLoss(double huber, double scale);

			void addTranslationConstraints(ceres::Problem &problem);

			void addRotationConstraints(ceres::Problem &problem);

			static double evaluate(ceres::Problem &problem,
								   const ceres::Problem::EvaluateOptions &evalOptions = ceres::Problem::EvaluateOptions());

			static ceres::Solver::Options setupOptions(bool logSummary);

			double evaluate(ceres::Problem &problem, const std::vector<ceres::ResidualBlockId> &blockIds);

			void evaluateCorrespondenceResiduals(ceres::Problem &problem);

			void evaluateWeightResiduals(ceres::Problem &problem);

			void evaluateLambdaResiduals(ceres::Problem &problem);

			void evaluateRotationResiduals(ceres::Problem &problem);

			void solveProblem(bool logSummary);

			void evaluateAllResiduals(ceres::Problem &problem);

		public:
			/**
			 * @constructor
			 *
			 * @param intrinsics The intrinsics of the pinhole camera model.
			 */
			explicit CameraPoseEstimator(Eigen::Matrix<double, 3, 4> intrinsics, bool initLogging = true,
										 double weightPenalizeScale = std::numeric_limits<double>::max());

			/**
			 * @destructor
			 */
			virtual ~CameraPoseEstimator() = default;

			void addWorldObject(const WorldObject &worldObject);

			void addWorldObjects(const std::vector<WorldObject> &vector);

			/**
			 * Estimates the camera translation and rotation based on the known correspondences between the world and
			 * image.
			 */
			void estimate(bool logSummary = false);

			std::thread estimateAsync(bool logSummary = false);

			/**
			 * Based on the known world positions calculates and initial guess for the camera translation and rotation.
			 * This is necessary as the optimization problem is rather ill posed and sensitive to the initialization.
			 */
			void calculateInitialGuess();

			void guessRotation(const Eigen::Vector3d &rotation);

			void guessTranslation(const Eigen::Vector3d &translation);

			/**
			 * @get
			 */
			const Eigen::Vector3d &getTranslation() const;

			/**
			 * @get
			 */
			const Eigen::Vector3d &getRotation() const;

			const std::vector<providentia::calibration::WorldObject> &getWorldObjects() const;

			bool isOptimizationFinished() const;

			friend std::ostream &operator<<(std::ostream &os, const CameraPoseEstimator &estimator);

			double getWeightPenalizeScale() const;

			void setWeightPenalizeScale(double weightPenalizeScale);

			void clearWorldObjects();

			std::vector<double> getWeights();

			double getLambdaPenalizeScale() const;

			void setLambdaPenalizeScale(double lambdaPenalizeScale);

			double getRotationPenalizeScale() const;

			void setRotationPenalizeScale(double rotationPenalizeScale);

			std::vector<double> getLambdas();

			bool hasFoundValidSolution() const;

			double getLambdasLoss() const;

			double getCorrespondencesLoss() const;

			double getRotationsLoss() const;

			double getWeightsLoss() const;

			double getTotalLoss() const;
		};
	}
}

#endif //CAMERASTABILIZATION_CAMERAPOSEESTIMATION_HPP
