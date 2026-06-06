import torch
import torch.nn as nn
import torch.nn.functional as F


class EmotionCNN(nn.Module):
    """TinyVGG-style 1D CNN for 4-class cognitive state classification.

    Input:  (batch, input_channels=14, time=125) — channel-first for Conv1d.
    Output: (batch, num_classes=4) logits.
    """

    def __init__(self, input_channels: int = 14, hidden_units: int = 10, num_classes: int = 4):
        super().__init__()
        self.block_1 = nn.Sequential(
            nn.Conv1d(input_channels, hidden_units, kernel_size=3, padding=1),
            nn.BatchNorm1d(hidden_units),
            nn.ReLU(),
            nn.Conv1d(hidden_units, hidden_units, kernel_size=3, padding=1),
            nn.ReLU(),
            nn.MaxPool1d(2),
        )
        self.block_2 = nn.Sequential(
            nn.Conv1d(hidden_units, hidden_units, 3, padding=1),
            nn.ReLU(),
            nn.BatchNorm1d(hidden_units),
            nn.Conv1d(hidden_units, hidden_units, 3, padding=1),
            nn.ReLU(),
            nn.MaxPool1d(2),
        )
        self.classifier = nn.Sequential(
            nn.Flatten(),
            nn.Linear(hidden_units * 31, num_classes),
        )

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.classifier(self.block_2(self.block_1(x)))


class CostSensitiveLoss(nn.Module):
    """Weighted cross-entropy using a predefined misclassification cost matrix.

    Penalises high-cost confusions (e.g. predicting stressed when truly relaxed)
    more heavily than low-cost confusions.
    """

    def __init__(self, cost_matrix):
        super().__init__()
        self.register_buffer('cost_matrix', torch.tensor(cost_matrix, dtype=torch.float32))

    def forward(self, logits: torch.Tensor, targets: torch.Tensor) -> torch.Tensor:
        probs = F.softmax(logits, dim=1)
        batch_costs = self.cost_matrix[targets]
        return torch.mean(torch.sum(batch_costs * probs, dim=1))
