# Load required libraries
library(readr)
library(data.table)
library(brms)
library(ggplot2)

# Data path and loading
data_path = "/home/pierre/ikaros/Source/Modules/RobotModules/CompliantRobot/CurrentPositionMapping/data/trajectories_data_std.csv"
std_data_table = fread(data_path)

setwd("/home/pierre/ikaros/Source/Modules/RobotModules/CompliantRobot/CurrentPositionMapping/Rwork/")

#split data into training and testing, by shuffling the indices and selecting 80% for training
set.seed(123)
train_indices <- sample(nrow(std_data_table), 0.8 * nrow(std_data_table))
train_data <- std_data_table[train_indices, ]
test_data <- std_data_table[-train_indices, ]


# Define file paths for saved models
tilt_model_path <- "tilt_model.rds"
tilt_quad_model_path <- "tilt_quad_model.rds"

# Fit or load the tilt model
if (!file.exists(tilt_model_path)) {
  tilt <- brm(
    tilt_current ~ tilt_pos + tilt_distance, 
    data = std_data_table, 
    family = gaussian(), 
    chains = 4, 
    iter = 2000, 
    cores = 10
  )
  saveRDS(tilt, file = tilt_model_path)
  print("Tilt model fitted and saved.")
} else {
  tilt <- readRDS(tilt_model_path)
  print("Tilt model loaded from file.")
}

# Fit or load the quadratic tilt model
if (!file.exists(tilt_quad_model_path)) {
  tilt_quad_model <- brm(
    tilt_current ~ poly(tilt_pos, 2) + tilt_distance,
    data = std_data_table,
    family = gaussian(),
    chains = 4,
    iter = 2000,
    cores = 10
  )
  saveRDS(tilt_quad_model, file = tilt_quad_model_path)
  print("Quadratic tilt model fitted and saved.")
} else {
  tilt_quad_model <- readRDS(tilt_quad_model_path)
  print("Quadratic tilt model loaded from file.")
}

# Select a small sample of data for posterior predictive checks
small_data <- std_data_table[sample(nrow(std_data_table), 5000), ]

# Define function for posterior predictive checks
posterior_check_multi <- function(models, model_names, data, response_var, ndraws = 100, plot_type = "dens_overlay") {
  results <- list()
  
  for (i in seq_along(models)) {
    model <- models[[i]]
    model_name <- model_names[i]
    
    # Generate posterior predictive samples
    posterior_preds <- posterior_predict(model, newdata = data, ndraws = ndraws)
    
    # Calculate summary statistics
    pred_means <- apply(posterior_preds, 2, mean)
    pred_intervals <- apply(posterior_preds, 2, quantile, probs = c(0.025, 0.975))
    observed <- data[[response_var]]
    rmse <- sqrt(mean((observed - pred_means)^2))
    
    # Create observed vs predicted plot with credible intervals
    obs_vs_pred_plot <- ggplot(data, aes(x = observed, y = pred_means)) +
      geom_point(color = "blue") +
      geom_abline(intercept = 0, slope = 1, color = "red") +
      geom_errorbar(aes(ymin = pred_intervals[1, ], 
      ymax = pred_intervals[2, ]), 
      width = 0.2, color = "gray") +
      labs(x = "Observed", y = "Predicted", title = paste("Observed vs Predicted:", model_name)) +
      theme_minimal()
    
    # Store results
    results[[model_name]] <- list(
      posterior_preds = posterior_preds,
      pred_means = pred_means,
      pred_intervals = pred_intervals,
      rmse = rmse,
      obs_vs_pred_plot = obs_vs_pred_plot
    )
    
    print(paste("Posterior check completed for model:", model_name))
  }
  
  return(results)
}
ndraws = 300
# Run posterior predictive checks for both models
models <- list(tilt, tilt_quad_model)
model_names <- c("tilt", "tilt_quad")
predictive_results <- posterior_check_multi(models, model_names, data = small_data, response_var = "tilt_current", ndraws = ndraws)
predictive_results$tilt$ppc_plot = ppc_check(tilt, ndraws = ndraws)
predictive_results$tilt_quad$ppc_plot = ppc_check(tilt_quad_model, ndraws = ndraws)

# Access the RMSE and plot for tilt_quad_model
print(predictive_results$tilt_quad$rmse)
print(predictive_results$tilt_quad$obs_vs_pred_plot)
