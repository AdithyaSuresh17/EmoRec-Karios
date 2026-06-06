"""Quantize the trained EmotionCNN to 8-bit integer weights using MASE.

Produces quantized_emotion_cnn.pth in the project root.

Usage:
    python scripts/quantize.py [--model-path <path>]
"""

import argparse

import torch

from emorec.model import EmotionCNN

MODEL_DEFAULT = 'Ml-Models/emotionv2_tiny_vgg_updated_windowing_14features.pth'


def parse_args():
    p = argparse.ArgumentParser(description='Quantize EmotionCNN with MASE.')
    p.add_argument('--model-path', default=MODEL_DEFAULT, help='Path to trained .pth weights')
    p.add_argument('--output', default='quantized_emotion_cnn.pth', help='Output path for quantized weights')
    return p.parse_args()


def main():
    args = parse_args()

    import chop
    import chop.passes as passes
    from chop import MaseGraph

    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    print(f'Device: {device}')

    model = EmotionCNN().to(device)
    model.load_state_dict(torch.load(args.model_path, map_location=device))

    dummy_input = torch.randn(1, 14, 125).to(device)
    mg = MaseGraph(model)
    mg, _ = passes.init_metadata_analysis_pass(mg)
    mg, _ = passes.add_common_metadata_analysis_pass(
        mg,
        pass_args={'dummy_in': {'x': dummy_input}, 'add_value': False},
    )

    quantization_config = {
        'by': 'type',
        'default': {'config': {'name': None}},
        'linear': {
            'config': {
                'name': 'integer',
                'weight_width': 8, 'weight_frac_width': 4,
                'data_in_width': 8, 'data_in_frac_width': 4,
                'bias_width': 8, 'bias_frac_width': 4,
            }
        },
        'conv1d': {
            'config': {
                'name': 'integer',
                'weight_width': 8, 'weight_frac_width': 4,
                'data_in_width': 8, 'data_in_frac_width': 4,
                'bias_width': 8, 'bias_frac_width': 4,
            }
        },
    }

    mg, _ = passes.quantize_transform_pass(mg, pass_args=quantization_config)
    quantized_model = mg.model

    print('Quantized architecture:')
    print(quantized_model)

    torch.save(quantized_model.state_dict(), args.output)
    print(f'\nSaved quantized model → {args.output}')


if __name__ == '__main__':
    main()
