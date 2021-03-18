//
// Created by brucknem on 04.02.21.
//

#include "Residuals.hpp"

#include <utility>
#include "glog/logging.h"

namespace providentia {
	namespace calibration {

#pragma region CorrespondenceResidualBase

		CorrespondenceResidual::CorrespondenceResidual(Eigen::Vector2d _expectedPixel,
													   std::shared_ptr<providentia::calibration::ParametricPoint> _point,
													   Eigen::Matrix<double, 3, 4> _intrinsics,
													   double maxLambda,
													   double _weight) :
			expectedPixel(std::move(_expectedPixel)),
			parametricPoint(std::move(_point)),
			intrinsics(std::move(_intrinsics)),
			maxLambda(maxLambda),
			weight(_weight) {}

		template<typename T>
		bool CorrespondenceResidual::calculateResidual(const T *_translation, const T *_rotation,
													   const T *_point, T *residual) const {
			Eigen::Matrix<T, 4, 1> point{_point[0], _point[1], _point[2], (T) 1};

//			std::cout << "Translation" << std::endl;
//			std::cout << _translation[0] << ", " << _translation[1] << ", " << _translation[2] << std::endl
//					  << std::endl;
//			std::cout << "Rotation" << std::endl;
//			std::cout << _rotation[0] << ", " << _rotation[1] << ", " << _rotation[2] << std::endl << std::endl;

			Eigen::Matrix<T, 2, 1> actualPixel;
			bool flipped;
			actualPixel = camera::render(_translation, _rotation, intrinsics, point.data(), flipped);

			residual[0] = expectedPixel.x() - actualPixel.x();
			residual[1] = expectedPixel.y() - actualPixel.y();

//			residual[0] *= (T) weight;
//			residual[1] *= (T) weight;

//			LOG(INFO) << std::endl << residual[0] << std::endl << residual[1];
			return !flipped;
		}

		template<typename T>
		bool CorrespondenceResidual::operator()(const T *_translation, const T *_rotation, const T *_lambda,
												const T *_mu, T *residual) const {
			Eigen::Matrix<T, 3, 1> point = parametricPoint->getOrigin().cast<T>();
//			std::cout << "Point" << std::endl;
//			std::cout << point << std::endl << std::endl;
			point += parametricPoint->getAxisA().cast<T>() * _lambda[0];
//			std::cout << point << std::endl << std::endl;
			point += parametricPoint->getAxisB().cast<T>() * _mu[0];
//			std::cout << point << std::endl << std::endl;
			bool result = calculateResidual(_translation, _rotation, point.data(), residual);

			// TODO add actual height from object from HD map.
			const T zero = (T) 0;
			T lambda = _lambda[0];
			if (lambda > (T) maxLambda) {
				lambda -= (T) maxLambda;
			} else if (lambda > zero) {
				lambda = zero;
			}

			residual[2] = lambda;
			residual[3] = zero;
//			_mu[0] * weight;

			return result;
		}

		ceres::CostFunction *
		CorrespondenceResidual::Create(const Eigen::Vector2d &_expectedPixel,
									   const std::shared_ptr<providentia::calibration::ParametricPoint> &_point,
									   const Eigen::Matrix<double, 3, 4> &_intrinsics,
									   double _maxLambda,
									   double _weight) {
			return new ceres::AutoDiffCostFunction<CorrespondenceResidual, 4, 3, 3, 1, 1>(
				new CorrespondenceResidual(_expectedPixel, _point, _intrinsics,
										   _maxLambda, _weight)
			);
		}

#pragma endregion CorrespondenceResidualBase
	}
}
