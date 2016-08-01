library(ggplot2)
library(scales)

read.snafu <- function(windows.file = "windows.dat", strings.file = "strings.dat") {
    f <- file(windows.file, "rb")
    f2 <- file(strings.file, "rb")

    x <- readBin(f, integer(), n = file.info(windows.file)$size)
    n <- length(x) / 2
    ns <- readBin(f2, integer())
    strings <- readBin(f2, "character", n = ns)

    data.frame(
        timestamp = as.POSIXct(bitwAnd(x[2*(1:n) - 1], bitwShiftR(-1, 1)), origin="1970-01-01", tz="UTC"),
        activity = bitwShiftR(x[2*(1:n) - 1], 31) == 1,
        window = subs[1 + x[2*(1:n)]])
}

categorize <- function(df) {
    df$category = "Unknown"
    df$category[df$window == "Terminal"] <- "Terminal"
    df$category[grepl("Chromium$", df$window)] <- "Other browser"
    df
}

df <- categorize(read.snafu())

ggplot(df, aes(x = category, fill = category)) + geom_bar(aes(y = ..count.. / sum(..count..))) + theme_bw() + scale_y_continuous(labels = percent) + xlab("Category") + ylab("Window")

