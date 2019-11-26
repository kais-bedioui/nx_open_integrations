// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "object_tracker_utils.h"

#include "geometry.h"

namespace sample_company {
namespace vms_server_plugins {
namespace opencv_object_detection {

using namespace cv;
using namespace cv::tbm;

using namespace nx::sdk;

Uuid IdMapper::get(int64_t id)
{
    const auto it = m_map.find(id);
    if (it == m_map.end())
    {
        Uuid result = UuidHelper::randomUuid();
        m_map[id] = result;
        return result;
    }
    return it->second;
}

void IdMapper::removeAllExcept(const std::set<int64_t>& idsToKeep)
{
    for (auto it = m_map.begin(); it != m_map.end(); ++it)
    {
        if (idsToKeep.find(it->first) == idsToKeep.end())
            m_map.erase(it);
    }
}

/**
 * Convert detections from the plugin format to the format of opencv::tbm, preserving classLabels.
 */
TrackedObjects convertDetectionsToTrackedObjects(
    const Frame& frame,
    const DetectionList& detections,
    ClassLabelMap* inOutClassLabels)
{
    TrackedObjects result;

    for (const std::shared_ptr<Detection>& detection: detections)
    {
        const cv::Rect cvRect = nxRectToCvRect(
            detection->boundingBox,
            frame.width,
            frame.height);

        inOutClassLabels->insert(std::make_pair(CompositeDetectionId{
            frame.index,
            cvRect},
            detection->classLabel));

        result.push_back(TrackedObject(
            cvRect,
            detection->confidence,
            (int) frame.index,
            /*object_id*/ -1)); //< Placeholder, to be filled in ObjectTracker::process().
    }

    return result;
}

/**
 * Convert detection from tbm format to our format, restoring the classLabels.
 */
std::shared_ptr<Detection> convertTrackedObjectToDetection(
    const Frame& frame,
    const TrackedObject& trackedDetection,
    const std::string& classLabel,
    IdMapper* idMapper)
{
    return std::make_shared<Detection>(Detection{
        /*boundingBox*/ cvRectToNxRect(trackedDetection.rect, frame.width, frame.height),
        classLabel,
        (float) trackedDetection.confidence,
        /*trackId*/ idMapper->get(trackedDetection.object_id)});
}

/**
 * Convert detections from opencv::tbm format to the plugin format, restoring classLabels.
 */
DetectionList convertTrackedObjectsToDetections(
    const Frame& frame,
    const TrackedObjects& trackedDetections,
    const ClassLabelMap& classLabels,
    IdMapper* idMapper)
{
    DetectionList result;
    for (const cv::tbm::TrackedObject& trackedDetection: trackedDetections)
    {
        const std::string classLabel = classLabels.at({
            frame.index,
            trackedDetection.rect});
        result.push_back(convertTrackedObjectToDetection(
            frame,
            trackedDetection,
            classLabel,
            idMapper));
    }

    return result;
}

DetectionList extractDetectionList(const DetectionInternalList& detectionsInternal)
{
    DetectionList result;
    for (const std::shared_ptr<DetectionInternal>& detection: detectionsInternal)
        result.push_back(detection->detection);
    return result;
}

} // namespace opencv_object_detection
} // namespace vms_server_plugins
} // namespace sample_company
