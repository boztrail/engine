// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#ifndef FLUTTER_FLOW_EMBEDDED_VIEWS_H_
#define FLUTTER_FLOW_EMBEDDED_VIEWS_H_

#include <vector>

#include "flutter/fml/memory/ref_counted.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/core/SkPoint.h"
#include "third_party/skia/include/core/SkRRect.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkSize.h"

namespace flutter {

enum MutatorType { clip_rect, clip_rrect, clip_path, transform };

// Stores mutation information like clipping or transform.
//
// The `type` indicates the type of the mutation: clip_rect, transform and etc.
// Each `type` is paired with an object that supports the mutation. For example,
// if the `type` is clip_rect, `rect()` is used the represent the rect to be
// clipped. One mutation object must only contain one type of mutation.
class Mutator {
 public:
  Mutator(const Mutator& other) {
    type_ = other.type_;
    switch (other.type_) {
      case clip_rect:
        rect_ = other.rect_;
        break;
      case clip_rrect:
        rrect_ = other.rrect_;
        break;
      case clip_path:
        path_ = new SkPath(*other.path_);
        break;
      case transform:
        matrix_ = other.matrix_;
        break;
      default:
        break;
    }
  }

  explicit Mutator(const SkRect& rect) : type_(clip_rect), rect_(rect) {}
  explicit Mutator(const SkRRect& rrect) : type_(clip_rrect), rrect_(rrect) {}
  explicit Mutator(const SkPath& path)
      : type_(clip_path), path_(new SkPath(path)) {}
  explicit Mutator(const SkMatrix& matrix)
      : type_(transform), matrix_(matrix) {}

  const MutatorType& type() const { return type_; }
  const SkRect& rect() const { return rect_; }
  const SkRRect& rrect() const { return rrect_; }
  const SkPath& path() const { return *path_; }
  const SkMatrix& matrix() const { return matrix_; }

  bool operator==(const Mutator& other) const {
    if (type_ != other.type_) {
      return false;
    }
    if (type_ == clip_rect && rect_ == other.rect_) {
      return true;
    }
    if (type_ == clip_rrect && rrect_ == other.rrect_) {
      return true;
    }
    if (type_ == clip_path && *path_ == *other.path_) {
      return true;
    }
    if (type_ == transform && matrix_ == other.matrix_) {
      return true;
    }

    return false;
  }

  bool operator!=(const Mutator& other) const { return !operator==(other); }

  bool isClipType() {
    return type_ == clip_rect || type_ == clip_rrect || type_ == clip_path;
  }

  ~Mutator() {
    if (type_ == clip_path) {
      delete path_;
    }
  };

 private:
  MutatorType type_;

  union {
    SkRect rect_;
    SkRRect rrect_;
    SkMatrix matrix_;
    SkPath* path_;
  };

};  // Mutator

// A stack of mutators that can be applied to an embedded platform view.
//
// The stack may include mutators like transforms and clips, each mutator
// applies to all the mutators that are below it in the stack and to the
// embedded view.
//
// For example consider the following stack: [T1, T2, T3], where T1 is the top
// of the stack and T3 is the bottom of the stack. Applying this mutators stack
// to a platform view P1 will result in T1(T2(T2(P1))).
class MutatorsStack {
 public:
  MutatorsStack() = default;

  void pushClipRect(const SkRect& rect);
  void pushClipRRect(const SkRRect& rrect);
  void pushClipPath(const SkPath& path);
  void pushTransform(const SkMatrix& matrix);

  // Removes the `Mutator` on the top of the stack
  // and destroys it.
  void pop();

  // Returns an iterator pointing to the top of the stack.
  const std::vector<std::shared_ptr<Mutator>>::const_reverse_iterator top()
      const;
  // Returns an iterator pointing to the bottom of the stack.
  const std::vector<std::shared_ptr<Mutator>>::const_reverse_iterator bottom()
      const;

  bool operator==(const MutatorsStack& other) const {
    if (vector_.size() != other.vector_.size()) {
      return false;
    }
    for (size_t i = 0; i < vector_.size(); i++) {
      if (*vector_[i] != *other.vector_[i]) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const MutatorsStack& other) const {
    return !operator==(other);
  }

 private:
  std::vector<std::shared_ptr<Mutator>> vector_;
};  // MutatorsStack

class EmbeddedViewParams {
 public:
  EmbeddedViewParams() = default;

  EmbeddedViewParams(const EmbeddedViewParams& other) {
    offsetPixels = other.offsetPixels;
    sizePoints = other.sizePoints;
    mutatorsStack = other.mutatorsStack;
  };

  SkPoint offsetPixels;
  SkSize sizePoints;
  MutatorsStack mutatorsStack;

  bool operator==(const EmbeddedViewParams& other) const {
    return offsetPixels == other.offsetPixels &&
           sizePoints == other.sizePoints &&
           mutatorsStack == other.mutatorsStack;
  }
};

// This is only used on iOS when running in a non headless mode,
// in this case ExternalViewEmbedder is a reference to the
// FlutterPlatformViewsController which is owned by FlutterViewController.
class ExternalViewEmbedder {
  // TODO(cyanglaz): Make embedder own the `EmbeddedViewParams`.

 public:
  ExternalViewEmbedder() = default;

  virtual void BeginFrame(SkISize frame_size) = 0;

  virtual void PrerollCompositeEmbeddedView(int view_id) = 0;

  virtual std::vector<SkCanvas*> GetCurrentCanvases() = 0;

  // Must be called on the UI thread.
  virtual SkCanvas* CompositeEmbeddedView(int view_id,
                                          const EmbeddedViewParams& params) = 0;

  virtual bool SubmitFrame(GrContext* context);

  virtual ~ExternalViewEmbedder() = default;

  FML_DISALLOW_COPY_AND_ASSIGN(ExternalViewEmbedder);

};  // ExternalViewEmbedder

}  // namespace flutter

#endif  // FLUTTER_FLOW_EMBEDDED_VIEWS_H_
