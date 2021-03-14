//
// Created by brucknem on 08.02.21.
//

#include "WorldObjects.hpp"

#include <utility>

namespace providentia {
	namespace calibration {
#pragma region Points

		ParametricPoint::ParametricPoint(Eigen::Vector3d _origin, const Eigen::Vector3d
		&_axisA, const Eigen::Vector3d &_axisB, double _lambda, double _mu, double _angle) :
			expectedPixel({0, 0}), origin(std::move(_origin)), axisA(_axisA.normalized()),
			axisB(_axisB.normalized()),
			lambda(_lambda), mu(_mu), angle(_angle) {}

		Eigen::Vector3d ParametricPoint::getPosition() const {
			return origin + lambda * axisA + mu * axisB;
		}

		const Eigen::Vector3d &ParametricPoint::getOrigin() const {
			return origin;
		}

		const Eigen::Vector3d &ParametricPoint::getAxisA() const {
			return axisA;
		}

		const Eigen::Vector3d &ParametricPoint::getAxisB() const {
			return axisB;
		}

		double *ParametricPoint::getLambda() {
			return &lambda;
		}

		double *ParametricPoint::getMu() {
			return &mu;
		}

		double *ParametricPoint::getAngle() {
			return &angle;
		}

		const Eigen::Vector2d &ParametricPoint::getExpectedPixel() const {
			return expectedPixel;
		}

		void ParametricPoint::setExpectedPixel(const Eigen::Vector2d &_expectedPixel) {
			expectedPixel = _expectedPixel;
			isExpectedPixelSet = true;
		}

		ParametricPoint ParametricPoint::OnCylinder(const Eigen::Vector2d &_expectedPixel, Eigen::Vector3d _origin,
													const Eigen::Vector3d &_axisA, const Eigen::Vector3d &_axisB,
													double _lambda, double _mu, double
													_angle) {
			ParametricPoint point = OnCylinder(std::move(_origin), _axisA, _axisB, _lambda, _mu, _angle);
			point.setExpectedPixel(_expectedPixel);
			return point;
		}

		ParametricPoint ParametricPoint::OnCylinder(Eigen::Vector3d _origin, const Eigen::Vector3d &_axisA,
													const Eigen::Vector3d &_axisB, double _lambda, double _mu, double
													_angle) {
			return ParametricPoint(std::move(_origin), _axisA.stableNormalized(), _axisB.stableNormalized(), _lambda,
								   _mu, _angle);
		}

		ParametricPoint ParametricPoint::OnLine(const Eigen::Vector2d &_expectedPixel, Eigen::Vector3d _origin, const
		Eigen::Vector3d &_heading, double _lambda) {
			ParametricPoint point = OnLine(std::move(_origin), _heading, _lambda);
			point.setExpectedPixel(_expectedPixel);
			return point;
		}

		ParametricPoint ParametricPoint::OnLine(Eigen::Vector3d _origin, const Eigen::Vector3d &_heading,
												double _lambda) {
			return ParametricPoint::OnCylinder(std::move(_origin), _heading, {0, 0, 0}, _lambda, 0);
		}

		ParametricPoint
		ParametricPoint::OnPoint(const Eigen::Vector2d &_expectedPixel, const Eigen::Vector3d &_worldPosition) {
			ParametricPoint point = OnPoint(_worldPosition);
			point.setExpectedPixel(_expectedPixel);
			return point;
		}

		ParametricPoint ParametricPoint::OnPoint(const Eigen::Vector3d &_worldPosition) {
			return ParametricPoint::OnLine(_worldPosition, {0, 0, 0}, 0);
		}

		bool ParametricPoint::hasExpectedPixel() const {
			return isExpectedPixelSet;
		}

#pragma endregion Points

#pragma region Objects

		void WorldObject::add(const ParametricPoint &point) {
			points.emplace_back(std::make_shared<ParametricPoint>(point));
		}

		double WorldObject::getWeight() const {
			if (points.empty()) {
				return 0;
			}
			return 1. / points.size();
		}

		const std::vector<std::shared_ptr<ParametricPoint>> &WorldObject::getPoints() const {
			return points;
		}

		WorldObject::WorldObject(const ParametricPoint &point) {
			add(point);
		}

		Eigen::Vector3d WorldObject::getMean() const {
			Eigen::Vector3d mean{0, 0, 0};
			for (const auto &point : points) {
				mean += point->getPosition();
			}
			return mean / points.size();
		}

		const std::string &WorldObject::getId() const {
			return id;
		}

		void WorldObject::setId(const std::string &_id) {
			id = _id;
		}

		double WorldObject::getHeight() const {
			return height;
		}

		void WorldObject::setHeight(double _height) {
			height = _height;
		}

		double WorldObject::getRadius() const {
			return radius;
		}

		void WorldObject::setRadius(double _radius) {
			radius = _radius;
		}

#pragma endregion Objects
	}
}