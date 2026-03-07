import torch
import torch.nn as nn
import torch.nn.functional as F
import numpy as np

class CostSensitiveLoss(nn.Module):
    def __init__(self, cost_matrix):
        super(CostSensitiveLoss, self).__init__()
        # Ensure the matrix is a PyTorch tensor on the correct device (CPU/GPU)
        self.cost_matrix = torch.tensor(cost_matrix, dtype=torch.float32)

    def forward(self, logits, targets):
        # 1. Convert the raw model outputs (logits) into percentages (0.0 to 1.0)
        probs = F.softmax(logits, dim=1)
        
        # 2. Match the penalty matrix device to the targets device
        self.cost_matrix = self.cost_matrix.to(targets.device)
        
        # 3. Pull the specific penalty rows for the true classes in this batch
        # If the target is 3 (Relaxed), it pulls the [1.0, 10.0, 1.0, 0.0] row
        batch_costs = self.cost_matrix[targets]
        
        # 4. Multiply the model's predicted probabilities by the penalties
        # If it predicted "Stressed" (column 1) with 90% confidence, 
        # the loss explodes: 0.90 * 10.0 penalty = Huge Loss!
        expected_costs = torch.sum(batch_costs * probs, dim=1)
        
        # 5. Return the average loss for the batch
        return torch.mean(expected_costs)
    



# Define your Risk Averse penalties
'''penalty_grid = [
    [0.0, 1.0, 1.0, 1.5],  # True: Distracted
    [1.0, 0.0, 1.0, 1.5],  # True: Focused
    [1.0, 1.0, 0.0, 1.5],  # True: Relaxed
    [1.0, 1.0, 1.0, 0.0]  # True: Stressed
]'''
# penalty_grid = [
#     [0.0, 1.0, 1.0, 1.5],  # True: Distracted
#     [1.0, 0.0, 1.0, 1.5],  # True: Focused
#     [1.0, 1.5, 0.0, 1.8],  # True: Relaxed
#     [1.0, 1.0, 1.0, 0.0]  # True: Stressed
# ]
penalty_grid = 0.05*np.array([
    [0.0, 1.0, 1.0, 1.5],  # True: Distracted
    [1.0, 0.0, 1.0, 1.5],  # True: Focused
    [1.0, 1.5, 0.0, 1.8],  # True: Relaxed
    [1.0, 1.0, 1.0, 0.0]  # True: Stressed
])