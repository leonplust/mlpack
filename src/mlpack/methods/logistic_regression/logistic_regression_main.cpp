/**
 * @file logistic_regression_main.cpp
 * @author Ryan Curtin
 *
 * Main executable for logistic regression.
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#include <mlpack/prereqs.hpp>
#include <mlpack/core/util/cli.hpp>
#include <mlpack/core/util/mlpack_main.hpp>

#include "logistic_regression.hpp"

#include <mlpack/core/optimizers/sgd/sgd.hpp>

using namespace std;
using namespace mlpack;
using namespace mlpack::regression;
using namespace mlpack::optimization;
using namespace mlpack::util;

PROGRAM_INFO("L2-regularized Logistic Regression and Prediction",
    "An implementation of L2-regularized logistic regression using either the "
    "L-BFGS optimizer or SGD (stochastic gradient descent).  This solves the "
    "regression problem"
    "\n\n"
    "  y = (1 / 1 + e^-(X * b))"
    "\n\n"
    "where y takes values 0 or 1."
    "\n\n"
    "This program allows loading a logistic regression model (via the " +
    PRINT_PARAM_STRING("input_model") + " parameter) "
    "or training a logistic regression model given training data (specified "
    "with the " + PRINT_PARAM_STRING("training") + " parameter), or both "
    "those things at once.  In addition, this program allows classification on "
    "a test dataset (specified with the " + PRINT_PARAM_STRING("test") + " "
    "parameter) and the classification results may be saved with the " +
    PRINT_PARAM_STRING("output") + " output parameter.  The trained logistic "
    "regression model may be saved using the " +
    PRINT_PARAM_STRING("output_model") + " output parameter."
    "\n\n"
    "The training data, if specified, may have class labels as its last "
    "dimension.  Alternately, the " + PRINT_PARAM_STRING("labels") + " "
    "parameter may be used to specify a separate matrix of labels."
    "\n\n"
    "When a model is being trained, there are many options.  L2 regularization "
    "(to prevent overfitting) can be specified with the " +
    PRINT_PARAM_STRING("lambda") + " option, and the "
    "optimizer used to train the model can be specified with the " +
    PRINT_PARAM_STRING("optimizer") + " parameter.  Available options are "
    "'sgd' (stochastic gradient descent) and 'lbfgs' (the L-BFGS optimizer).  "
    "There are also various parameters for the optimizer; the " +
    PRINT_PARAM_STRING("max_iterations") + " parameter specifies the maximum "
    "number of allowed iterations, and the " +
    PRINT_PARAM_STRING("tolerance") + " parameter specifies the tolerance for "
    " convergence.  For the SGD optimizer, the " +
    PRINT_PARAM_STRING("step_size") + " parameter controls the step size taken"
    " at each iteration by the optimizer.  The batch size for SGD is controlled"
    " with the " + PRINT_PARAM_STRING("batch_size") + " parameter. If the "
    "objective function for your data is oscillating between Inf and 0, the "
    "step size is probably too large.  There are more parameters for the "
    "optimizers, but the C++ interface must be used to access these."
    "\n\n"
    "For SGD, an iteration refers to a single point. So to take a single pass "
    "over the dataset with SGD, " + PRINT_PARAM_STRING("max_iterations") +
    " should be set to the number of points in the dataset."
    "\n\n"
    "Optionally, the model can be used to predict the responses for another "
    "matrix of data points, if " + PRINT_PARAM_STRING("test") + " is "
    "specified.  The " + PRINT_PARAM_STRING("test") + " parameter can be "
    "specified without the " + PRINT_PARAM_STRING("training") + " parameter, "
    "so long as an existing logistic regression model is given with the " +
    PRINT_PARAM_STRING("input_model") + " parameter.  The output predictions "
    "from the logistic regression model may be saved with the " +
    PRINT_PARAM_STRING("output") + " parameter."
    "\n\n"
    "This implementation of logistic regression does not support the general "
    "multi-class case but instead only the two-class case.  Any labels must "
    "be either 0 or 1.  For more classes, see the softmax_regression program."
    "\n\n"
    "As an example, to train a logistic regression model on the data '" +
    PRINT_DATASET("data") + "' with labels '" + PRINT_DATASET("labels") + "' "
    "with L2 regularization of 0.1, saving the model to '" +
    PRINT_MODEL("lr_model") + "', the following command may be used:"
    "\n\n" +
    PRINT_CALL("logistic_regression", "training", "data", "labels", "labels",
        "lambda", 0.1, "output_model", "lr_model") +
    "\n\n"
    "Then, to use that model to predict classes for the dataset '" +
    PRINT_DATASET("test") + "', storing the output predictions in '" +
    PRINT_DATASET("predictions") + "', the following command may be used: "
    "\n\n" +
    PRINT_CALL("logistic_regression", "input_model", "lr_model", "test", "test",
        "output", "predictions"));

// Training parameters.
PARAM_MATRIX_IN("training", "A matrix containing the training set (the matrix "
    "of predictors, X).", "t");
PARAM_UROW_IN("labels", "A matrix containing labels (0 or 1) for the points "
    "in the training set (y).", "l");

// Optimizer parameters.
PARAM_DOUBLE_IN("lambda", "L2-regularization parameter for training.", "L",
    0.0);
PARAM_STRING_IN("optimizer", "Optimizer to use for training ('lbfgs' or "
    "'sgd').", "O", "lbfgs");
PARAM_DOUBLE_IN("tolerance", "Convergence tolerance for optimizer.", "e",
    1e-10);
PARAM_INT_IN("max_iterations", "Maximum iterations for optimizer (0 indicates "
    "no limit).", "n", 10000);
PARAM_DOUBLE_IN("step_size", "Step size for SGD optimizer.",
    "s", 0.01);
PARAM_INT_IN("batch_size", "Batch size for SGD.", "b", 64);

// Model loading/saving.
PARAM_MODEL_IN(LogisticRegression<>, "input_model", "Existing model "
    "(parameters).", "m");
PARAM_MODEL_OUT(LogisticRegression<>, "output_model", "Output for trained "
    "logistic regression model.", "M");

// Testing.
PARAM_MATRIX_IN("test", "Matrix containing test dataset.", "T");
PARAM_UROW_OUT("output", "If test data is specified, this matrix is where "
    "the predictions for the test set will be saved.", "o");
PARAM_MATRIX_OUT("output_probabilities", "If test data is specified, this "
    "matrix is where the class probabilities for the test set will be saved.",
    "p");
PARAM_DOUBLE_IN("decision_boundary", "Decision boundary for prediction; if the "
    "logistic function for a point is less than the boundary, the class is "
    "taken to be 0; otherwise, the class is 1.", "d", 0.5);

static void mlpackMain()
{
  // Collect command-line options.
  const double lambda = CLI::GetParam<double>("lambda");
  const string optimizerType = CLI::GetParam<string>("optimizer");
  const double tolerance = CLI::GetParam<double>("tolerance");
  const double stepSize = CLI::GetParam<double>("step_size");
  const size_t batchSize = (size_t) CLI::GetParam<int>("batch_size");
  const size_t maxIterations = (size_t) CLI::GetParam<int>("max_iterations");
  const double decisionBoundary = CLI::GetParam<double>("decision_boundary");

  // One of training and input_model must be specified.
  RequireAtLeastOnePassed({ "training", "input_model" }, true);

  // If no output file is given, the user should know that the model will not be
  // saved, but only if a model is being trained.
  if (CLI::HasParam("training"))
  {
    RequireAtLeastOnePassed({ "output_model" }, false, "trained model will not "
        "be saved");
  }

  RequireAtLeastOnePassed({ "output_model", "output", "output_probabilities" },
      false, "no output will be saved");

  ReportIgnoredParam({{ "test", false }}, "output");
  ReportIgnoredParam({{ "test", false }}, "output_probabilities");

  // Tolerance needs to be positive.
  RequireParamValue<double>("tolerance", [](double x) { return x >= 0.0; },
      true, "tolerance must be positive or zero");

  // Optimizer has to be L-BFGS or SGD.
  RequireParamInSet<string>("optimizer", { "lbfgs", "sgd" },
      true, "unknown optimizer");

  // Lambda must be positive.
  RequireParamValue<double>("lambda", [](double x) { return x >= 0.0; },
      true, "lambda must be positive or zero");

  // Decision boundary must be between 0 and 1.
  RequireParamValue<double>("decision_boundary",
      [](double x) { return x >= 0.0 && x <= 1.0; }, true,
      "decision boundary must be between 0.0 and 1.0");

  RequireParamValue<double>("step_size", [](double x) { return x >= 0.0; },
      true, "step size must be positive");

  if (optimizerType != "sgd")
  {
    if (CLI::HasParam("step_size"))
    {
      Log::Warn << PRINT_PARAM_STRING("step_size") << " ignored because "
          << "optimizer type is not 'sgd'." << std::endl;
    }
    if (CLI::HasParam("batch_size"))
    {
      Log::Warn << PRINT_PARAM_STRING("batch_size") << " ignored because "
          << "optimizer type is not 'sgd'." << std::endl;
    }
  }

  // These are the matrices we might use.
  arma::mat regressors;
  arma::Row<size_t> responses;
  arma::mat testSet;
  arma::Row<size_t> predictions;

  // Load data matrix.
  if (CLI::HasParam("training"))
    regressors = std::move(CLI::GetParam<arma::mat>("training"));

  // Load the model, if necessary.
  LogisticRegression<>* model;
  if (CLI::HasParam("input_model"))
    model = CLI::GetParam<LogisticRegression<>*>("input_model");
  else
  {
    model = new LogisticRegression<>(0, 0);

    // Set the size of the parameters vector, if necessary.
    if (!CLI::HasParam("labels"))
      model->Parameters() = arma::zeros<arma::rowvec>(regressors.n_rows);
    else
      model->Parameters() = arma::zeros<arma::rowvec>(regressors.n_rows + 1);
  }

  // Check if the responses are in a separate file.
  if (CLI::HasParam("training") && CLI::HasParam("labels"))
  {
    responses = std::move(CLI::GetParam<arma::Row<size_t>>("labels"));
    if (responses.n_cols != regressors.n_cols)
      Log::Fatal << "The labels must have the same number of points as the "
          << "training dataset." << endl;
  }
  else if (CLI::HasParam("training"))
  {
    // The initial predictors for y, Nx1.
    responses = arma::conv_to<arma::Row<size_t>>::from(
        regressors.row(regressors.n_rows - 1));
    regressors.shed_row(regressors.n_rows - 1);
  }

  // Verify the labels.
  if (CLI::HasParam("training") && max(responses) > 1)
    Log::Fatal << "The labels must be either 0 or 1, not " << max(responses)
        << "!" << endl;

  // Now, do the training.
  if (CLI::HasParam("training"))
  {
    model->Lambda() = lambda;

    if (optimizerType == "sgd")
    {
      SGD<> sgdOpt;
      sgdOpt.MaxIterations() = maxIterations;
      sgdOpt.Tolerance() = tolerance;
      sgdOpt.StepSize() = stepSize;
      sgdOpt.BatchSize() = batchSize;
      Log::Info << "Training model with SGD optimizer." << endl;

      // This will train the model.
      model->Train(regressors, responses, sgdOpt);
    }
    else if (optimizerType == "lbfgs")
    {
      L_BFGS lbfgsOpt;
      lbfgsOpt.MaxIterations() = maxIterations;
      lbfgsOpt.MinGradientNorm() = tolerance;
      Log::Info << "Training model with L-BFGS optimizer." << endl;

      // This will train the model.
      model->Train(regressors, responses, lbfgsOpt);
    }
  }

  if (CLI::HasParam("test"))
  {
    testSet = std::move(CLI::GetParam<arma::mat>("test"));

    // We must perform predictions on the test set.  Training (and the
    // optimizer) are irrelevant here; we'll pass in the model we have.
    if (CLI::HasParam("output"))
    {
      Log::Info << "Predicting classes of points in '"
          << CLI::GetPrintableParam<arma::mat>("test") << "'." << endl;
      model->Classify(testSet, predictions, decisionBoundary);

      CLI::GetParam<arma::Row<size_t>>("output") = std::move(predictions);
    }

    if (CLI::HasParam("output_probabilities"))
    {
      Log::Info << "Calculating class probabilities of points in '"
          << CLI::GetPrintableParam<arma::mat>("test") << "'." << endl;
      arma::mat probabilities;
      model->Classify(testSet, probabilities);

      CLI::GetParam<arma::mat>("output_probabilities") =
          std::move(probabilities);
    }
  }

  CLI::GetParam<LogisticRegression<>*>("output_model") = model;
}
