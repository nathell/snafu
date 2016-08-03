library(ggplot2)
library(scales)

find.inactivity <- function(df, threshold = 5 * 60) {
    r <- rle(df$activity)
    pos <- which(r$lengths >= threshold & !r$values)
    ofs <- c(0, cumsum(r$lengths))
    res <- data.frame(start = ofs[pos] + 1,
                      count = r$lengths[pos])
    res$starttime <- df$timestamp[res$start]
    res$length <- difftime(df$timestamp[res$start + res$count - 1], df$timestamp[res$start])
    res
}

remove.inactivity <- function(df, inactive = find.inactivity(df)) {
    rows.to.remove <- integer()
    for (i in 1:nrow(inactive)) {
        rows.to.remove <- c(rows.to.remove, inactive$start[i]:(inactive$start[i] + inactive$count[i] - 1))
    }
    df[-rows.to.remove,]
}

read.snafu <- function(windows.file = "windows.dat", strings.file = "strings.dat") {
    f <- file(windows.file, "rb")
    f2 <- file(strings.file, "rb")

    x <- readBin(f, integer(), n = file.info(windows.file)$size / 4)
    n <- length(x) / 2
    ns <- readBin(f2, integer())
    strings <- readBin(f2, "character", n = ns)

    data.frame(
        timestamp = as.POSIXct(bitwAnd(x[2*(1:n) - 1], bitwShiftR(-1, 1)), origin="1970-01-01", tz="UTC"),
        activity = bitwShiftR(x[2*(1:n) - 1], 31) == 1,
        window = strings[1 + x[2*(1:n)]])
}

categorize <- function(df) {
    df$category = "Unknown"
    df$category[df$window == "Terminal"] <- "Terminal"
    df$category[grepl("Facebook", df$window)] <- "Facebook"
    df$category[grepl("Chromium$", df$window)] <- "Other browser"
    df$category[grepl("Firefox$", df$window)] <- "Other browser"
    df$category[grepl("^emacs", df$window)] <- "Emacs"
    df
}

df <- categorize(read.snafu())

plot.activity <- function(df) {
    ggplot(df, aes(x = category, fill = category)) +
        geom_bar(aes(y = ..count.. / sum(..count..))) +
        theme_bw() +
        scale_y_continuous(labels = percent) +
        xlab("Category") +
        ylab("Window")
}

plot.activity(remove.inactivity(df))
