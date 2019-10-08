from config import get_default_cfgs
import oneflow as flow
import oneflow.core.operator.op_conf_pb2 as op_conf_util

from backbone import Backbone
from rpn import RPNHead, RPNLoss, RPNProposal
from box_head import BoxHead
from mask_head import MaskHead

import argparse

parser = argparse.ArgumentParser()
parser.add_argument(
    "--config_file", "-c", default=None, type=str, help="yaml config file"
)
parser.add_argument(
    "-load", "--model_load_dir", type=str, default="", required=False
)
parser.add_argument(
    "-g", "--gpu_num_per_node", type=int, default=1, required=False
)
parser.add_argument(
    "-d",
    "--debug",
    type=bool,
    default=True,
    required=False,
    help="debug with random data generated by numpy",
)
parser.add_argument(
    "-rpn", "--rpn_only", default=False, action="store_true", required=False
)

args = parser.parse_args()


def get_numpy_placeholders():
    import numpy as np

    (N, H, W, C) = (2, 64, 64, 3)
    R = 50
    G = 12
    return {
        "images": np.random.randn(N, H, W, C).astype(np.float32),
        "image_sizes": np.random.randn(N, 2).astype(np.int32),
        "gt_boxes": np.random.randn(N, R, 4).astype(np.float32),
        "gt_segms": np.random.randn(N, G, 28, 28).astype(np.int8),
        "gt_labels": np.random.randn(N, G).astype(np.int32),
    }


placeholders = get_numpy_placeholders()


def maskrcnn(images, image_sizes, gt_boxes, gt_segms, gt_labels):
    # def maskrcnn(images, image_sizes, gt_boxes, gt_segms, gt_labels):
    r"""Mask-RCNN
    Args:
    images: (N, H, W, C)
    image_sizes: (N, 2)
    gt_boxes: (N, R, 4), dynamic
    gt_segms: (N, G, 28, 28), dynamic
    gt_labels: (N, G), dynamic
    """
    cfg = get_default_cfgs()
    if args.config_file is not None:
        cfg.merge_from_file(args.config_file)
    cfg.freeze()
    print(cfg)
    backbone = Backbone(cfg)
    rpn_head = RPNHead(cfg)
    rpn_loss = RPNLoss(cfg)
    rpn_proposal = RPNProposal(cfg)
    box_head = BoxHead(cfg)
    mask_head = MaskHead(cfg)

    image_size_list = flow.piece_slice(
        image_sizes, cfg.TRAINING_CONF.IMG_PER_GPU
    )
    gt_boxes_list = flow.piece_slice(gt_boxes, cfg.TRAINING_CONF.IMG_PER_GPU)
    gt_labels_list = flow.piece_slice(gt_labels, cfg.TRAINING_CONF.IMG_PER_GPU)
    gt_segms_list = flow.piece_slice(gt_segms, cfg.TRAINING_CONF.IMG_PER_GPU)
    anchors = []
    for i in range(cfg.DECODER.FPN_LAYERS):
        anchors.append(
            flow.detection.anchor_generate(
                images=images,
                feature_map_stride=cfg.DECODER.FEATURE_MAP_STRIDE * pow(2, i),
                aspect_ratios=cfg.DECODER.ASPECT_RATIOS,
                anchor_scales=cfg.DECODER.ANCHOR_SCALES * pow(2, i),
            )
        )

    # Backbone
    # CHECK_POINT: fpn features
    features = backbone.build(images)

    # RPN
    cls_logit_list, bbox_pred_list = rpn_head.build(features)
    rpn_bbox_loss, rpn_objectness_loss = rpn_loss.build(
        anchors, image_size_list, gt_boxes_list, bbox_pred_list, cls_logit_list
    )
    if args.rpn_only:
        return rpn_bbox_loss, rpn_objectness_loss

    proposals = rpn_proposal.build(
        anchors, cls_logit_list, bbox_pred_list, image_size_list, gt_boxes_list
    )

    # Box Head
    box_loss, cls_loss, pos_proposal_list, pos_gt_indices_list = box_head.build_train(
        proposals, gt_boxes_list, gt_labels_list, features
    )

    # Mask Head
    mask_loss = mask_head.build_train(
        pos_proposal_list,
        pos_gt_indices_list,
        gt_segms_list,
        gt_labels_list,
        features,
    )

    return rpn_bbox_loss, rpn_objectness_loss, box_loss, cls_loss, mask_loss


@flow.function
def debug_train(
    images=flow.input_blob_def(
        placeholders["images"].shape, dtype=flow.float32
    ),
    image_sizes=flow.input_blob_def(
        placeholders["image_sizes"].shape, dtype=flow.int32
    ),
    gt_boxes=flow.input_blob_def(
        placeholders["gt_boxes"].shape, dtype=flow.float32
    ),
    gt_segms=flow.input_blob_def(
        placeholders["gt_segms"].shape, dtype=flow.int8
    ),
    gt_labels=flow.input_blob_def(
        placeholders["gt_labels"].shape, dtype=flow.int32
    ),
):
    flow.config.train.primary_lr(0.00001)
    flow.config.train.model_update_conf(dict(naive_conf={}))
    outputs = maskrcnn(images, image_sizes, gt_boxes, gt_segms, gt_labels)
    for loss in outputs:
        flow.losses.add_loss(loss)
    return outputs


@flow.function
def debug_eval(
    images=flow.input_blob_def(
        placeholders["images"].shape, dtype=flow.float32
    ),
    image_sizes=flow.input_blob_def(
        placeholders["image_sizes"].shape, dtype=flow.int32
    ),
    gt_boxes=flow.input_blob_def(
        placeholders["gt_boxes"].shape, dtype=flow.float32
    ),
    gt_segms=flow.input_blob_def(
        placeholders["gt_segms"].shape, dtype=flow.int8
    ),
    gt_labels=flow.input_blob_def(
        placeholders["gt_labels"].shape, dtype=flow.int32
    ),
):
    outputs = maskrcnn(images, image_sizes, gt_boxes, gt_segms, gt_labels)
    return outputs


if __name__ == "__main__":
    flow.config.gpu_device_num(args.gpu_num_per_node)
    flow.config.ctrl_port(19788)

    flow.config.default_data_type(flow.float)
    check_point = flow.train.CheckPoint()
    if not args.model_load_dir:
        check_point.init()
    else:
        check_point.load(args.model_load_dir)
    if args.debug:
        train_loss = debug_train(
            placeholders["images"],
            placeholders["image_sizes"],
            placeholders["gt_boxes"],
            placeholders["gt_segms"],
            placeholders["gt_labels"],
        ).get()
        print(train_loss)
        eval_loss = debug_eval(
            placeholders["images"],
            placeholders["image_sizes"],
            placeholders["gt_boxes"],
            placeholders["gt_segms"],
            placeholders["gt_labels"],
        ).get()
        print(eval_loss)
